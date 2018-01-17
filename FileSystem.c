//
//  FileSystem.c
//  File System
//
//  Created by Liz Lin on 10/18/17.
//  Copyright © 2017 Liz Lin. All rights reserved.
//


#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<fcntl.h>
#include<string.h>


struct OFT
{
    char block[64];
    int cur_pos;
    int des_index;
    int file_len;
};

struct OFT oft[4];
char ldisk[64][64];
FILE* out_file;
unsigned int MASK[32];
unsigned int NMASK[32];

char mem_buff[200];//for constructing write buf
char mem_area[200];//for user_read and user_write


int char_arr_to_int(int i, int j)
{
    char temp[4];
    for(int k=0; k<4; ++k)
        temp[k] = ldisk[i][j+k];
    int num = *((int*) temp);
    return num;
}

int char_arr_to_int_oft(int i)
{
    char temp[4];
    for(int k=0; k<4; ++k)
        temp[k] = oft[0].block[i+k];
    int num = *((int*) temp);
    return num;
}
void int_to_char_arr(int i, int j, int num)
{
    char temp[4];
    *((int *) temp) = num;
    for(int k=0; k<4; ++k)
        ldisk[i][j+k]= temp[k];
}
void int_to_char_arr_oft(int i, int num)
{
    char temp[4];
    *((int *) temp) = num;
    for(int k=0; k<4; ++k)
        oft[0].block[i+k]= temp[k];
}
void increase_by_one(int i, int j)
{
    int num = char_arr_to_int(i, j);
    int_to_char_arr(i, j, ++num);
}

void decrease_by_one(int i, int j)
{
    int num = char_arr_to_int(i, j);
    int_to_char_arr(i, j, --num);
}
unsigned int char_arr_to_un_int(int i, int j)
{
    char temp[4];
    for(int k=0; k<4; ++k)
        temp[k] = ldisk[i][j+k];
    unsigned int num = *((unsigned int*) temp);
    return num;
}

void un_int_to_char_arr(int i, int j, unsigned int num)
{
    char temp[4];
    *((unsigned int *) temp) = num;
    for(int k=0; k<4; ++k)
        ldisk[i][j+k]= temp[k];
}

void ini_bitmap()
{
    *((unsigned int *) ldisk[0]) = 0;
    *((unsigned int *) ldisk[0]) = 0xFE000000;
}
void build_MASK()
{
    MASK[0]= 0x80000000;
    for(int i=1; i < 32; ++i)
        MASK[i] = MASK[i-1]>> 1;//printf("%u\n", MASK[i]);
    for(int i=0; i < 32; ++i)
        NMASK[i] = ~MASK[i]; //printf("%u\n", NMASK[i]);
}

void set_bit_to_0(int block_index)
{
    if(block_index <= 32) //first integer
    {
        unsigned int num = char_arr_to_un_int(0, 0);
        num = num & NMASK[block_index];
        un_int_to_char_arr(0, 0, num);
    }
    else
    {
        unsigned int num = char_arr_to_un_int(0, 4);
        num = num & NMASK[block_index-32];
        un_int_to_char_arr(0, 4, num);
    }
}

void set_bit_to_1(int block_index)
{
    if(block_index <= 32) //first integer
    {
        unsigned int num = char_arr_to_un_int(0, 0);
        num = num | MASK[block_index];
        un_int_to_char_arr(0, 0, num);
    }
    else
    {
        unsigned int num = char_arr_to_un_int(0, 4);
        num = num | MASK[block_index-32];
        un_int_to_char_arr(0, 4, num);
    }
}
void clear_oft_table()
{
    for(int i=0; i<4; ++i)
    {
        oft[i].des_index = -1;
        for(int j=0; j<64; ++j)
            oft[i].block[j] = '\0';
    }
}
/////////////////////////////////////////////////////////////////////////////
void read_block(int block_num, int oft_index)
{
    for(int i=0; i<64; ++i )
        oft[oft_index].block[i] = ldisk[block_num][i];
}

void write_block(int block_num, int oft_index)//FS TO IO
{
    for(int i=0; i<64; ++i )
        ldisk[block_num][i] = oft[oft_index].block[i];
    
}
int find_empty_block()
{
    unsigned int num1 = char_arr_to_un_int(0, 0);
    for(int k=7; k<32; ++k)
        if((num1 & MASK[k]) == 0)
        {
            set_bit_to_1(k);
            return k;
        }
    
    unsigned int num2 = char_arr_to_un_int(0, 4);
    for(int k=0; k<32; ++k)
        if((num2 & MASK[k]) == 0)
        {
            set_bit_to_1(k+32);
            return (k+32);
        }
    
    fprintf(out_file,"error\n");
    return -1;//no empty block
}


// 0 16 32 48
void build_file_descriptor()
{
    for(int i=1; i<=6 ; ++i)// make every file descriptor's len = -1
        for(int j=0; j<16; ++j)
            int_to_char_arr(i, 4*j, -1);
}

int find_empty_file_descriptor()
{
    for(int i=1; i<=6 ; ++i)
    {
        for(int j=0; j<4; ++j)
        {
            if(i ==1 && j== 0)
                continue;
            if(char_arr_to_int(i ,16* j) == -1)
            {
                int des = (--i)*4+j;
                return des;
            }
        }
    }
    return -1;
}

void build_directory_by_block_num(int block_num)
{
    for(int i= 0; i <8; ++i)
        int_to_char_arr(block_num,8*i+4, -1);
}

int expand_directory_block(int order)
{
    int empty_block = find_empty_block();//fine empty block;
    set_bit_to_1(empty_block); //indicate that block is in use
    int_to_char_arr(1, 4*order, empty_block); //update directory's file descriptor
    build_directory_by_block_num(empty_block); // make that block's file descriptor index all = -1
    read_block(empty_block,0);// oft[0].block);
    oft[0].des_index = 1;
    oft[0].cur_pos = order;
    return empty_block;
}
void ini_directory()
{
    expand_directory_block(1);//expand the first directory block
    int_to_char_arr(1, 0, 0);
    int_to_char_arr(1, 8, -1);
    int_to_char_arr(1, 12, -1);
}

int compare_file_name_oft( int i, char* name)
{
    if((oft[0].block[i] == name[0]) && (oft[0].block[i+1] == name[1]) && (oft[0].block[i+2] == name[2]))
        return 1;//exist
    else
        return 0;
    
}

void print_block(int index)
{
    for(int i=0; i<64; ++i)
        printf("%c", ldisk[index][i]);
    printf("end of block: %d\n", index);
}
void copy_name_to_oft(int index, char* name)
{
    for(int i=0; i<3; ++i)
        oft[0].block[index+i] = name[i];
    oft[0].block[index+3] = '\0';
}
int check_file_exist(char* name)
{
    int block=0;
    write_block(char_arr_to_int(1,4*oft[0].cur_pos), 0);
    for(int order=1; order<=3; ++order)
    {
        if( (block = char_arr_to_int(1, 4*order)) != -1)
        {
            read_block(block,0);// oft[0].block);
            oft[0].cur_pos = order;
            for(int i=0; i<8;++i)
                if(char_arr_to_int_oft(8*i+4) != -1)
                    if(compare_file_name_oft(8*i, name) == 1)//does exist
                        return 1;
        }
    }
    return 0;
}

int find_empty_directory_and_set(char *name, int descriptor_index)
{//set file len to 0;
    int block=0;
    write_block(char_arr_to_int(1,4*oft[0].cur_pos), 0);
    for(int order=1; order<=3; ++order)
    {
        if( (block = char_arr_to_int(1, 4*order)) != -1)
        {
            read_block(block,0);// oft[0].block);
            oft[0].cur_pos = order;
            for(int i=0; i<8;++i)
            {
                if(char_arr_to_int_oft(8*i+4) == -1)
                {
                    copy_name_to_oft(8*i, name);
                    int_to_char_arr_oft(8*i+4, descriptor_index);
                    write_block(block, 0);
                    increase_by_one(1, 0);//increase num of file
                    return 1;
                }
            }
        }
        else
        {
            block = expand_directory_block(order);
            increase_by_one(1, 0);
            copy_name_to_oft(0, name);
            int_to_char_arr_oft(4, descriptor_index);
            write_block(block, 0);
            return 1;
        }
    }
    return  0 ;
}

int get_i(int file_des)
{
    return ((int)(file_des/4)+1)  ;
}
int get_j(int file_des)
{
    return ((int)(file_des%4))*16;
}

void create(char* file_name)//,FILE* out_file)
{
    if( check_file_exist(file_name) == 1)
        fprintf(out_file, "error\n");
    else
    {
        int file_des = find_empty_file_descriptor();
        if ((file_des != -1 )&& (find_empty_directory_and_set(file_name, file_des) ==1))
        {
            int_to_char_arr(get_i(file_des), get_j(file_des), 0);
            //increase_by_one(1, 0);
            fprintf(out_file, "%s created\n",file_name);
        }
        else
            fprintf(out_file, "error\n");
    }
}



int find_file_in_directory(char* name, int isDes)
{
    int block=0, file_des =0;
    write_block(char_arr_to_int(1,4*oft[0].cur_pos),0);
    for(int order=1; order<=3; ++order)
    {
        if( (block = char_arr_to_int(1, 4*order)) != -1)
        {
            read_block(block, 0);//oft[0].block);
            oft[0].cur_pos = order;
            for(int i=0; i<8;++i)
            {
                if((char_arr_to_int_oft(8*i+4) != -1) && compare_file_name_oft(8*i, name) == 1 )
                {
                    file_des = char_arr_to_int_oft(8*i+4);
                    if(isDes == 1)
                    {
                        int_to_char_arr_oft(8*i+4, -1);
                        write_block(block,0);
                    }
                    return file_des;
                }
            }
        }
    }
    return -1;
}
void free_block(int block)
{
    for(int i=0; i<64; ++i)
        ldisk[block][i] = '\0';
    
}
int destory(char* file_name)//make directory's file_des
{
    int file_des = find_file_in_directory(file_name,1);
    if(file_des != -1 )
    {
        int i = get_i(file_des), j = get_j(file_des);
        int_to_char_arr(i,j, -1);//make len = -1
        for(int k=1; k<4; ++k)
        {
            int block = char_arr_to_int(i, j+4*k);
            if(block != -1)
            {
                free_block(block);
                set_bit_to_0(block);
                int_to_char_arr(i, j+4*k, -1);
            }
        }
        decrease_by_one(1, 0);
        for(int i=1; i<4; ++i)
        {
            if(oft[i].des_index == file_des)
            {
                oft[i].des_index = -1;
            }
        }
        fprintf(out_file, "%s destroyed\n",file_name);
        return 1;
    }
    else
    {
        fprintf(out_file, "error\n");
        return 0;
    }
}

int get_block(int cur_pos)
{
    int cur_block = 0;
    if(cur_pos <64 )
        cur_block = 1;
    else if (cur_pos < 128)
        cur_block = 2;
    else
        cur_block =3;
    
    return cur_block;
}
void ini_and_clear_mem_area()
{
    for(int i=0; i<200; ++i)
        mem_area[i] = '\0';
}
int user_read(int index, int count)//user to FS
{
    int mem_index=0;
    int cur_pos =oft[index].cur_pos, file_len = oft[index].file_len,file_des = oft[index].des_index;
    int cur_block_order = get_block(cur_pos);
    int end_block_order = get_block(cur_pos + count);

    ini_and_clear_mem_area();
    if (cur_block_order == end_block_order)
    {
        while( mem_index <count && cur_pos < file_len)
            mem_area[mem_index++] = oft[index].block[cur_pos++%64];
        mem_area[mem_index] = '\0';
        oft[index].cur_pos = cur_pos;
    }
    else
    {
        int left_over = 64 - cur_pos %64;
        while( mem_index<left_over)
            mem_area[mem_index++] = oft[index].block[cur_pos++%64];
        count -= mem_index;
        while(count >0 && cur_pos < file_len)//>=0
        {
            int next_block= char_arr_to_int(get_i(file_des), get_j(file_des) +4*(++cur_block_order));
            read_block(next_block, index);//read this new block to oft buff(end)
            int i =0;
            for(; i < count && i<64 && (cur_pos < oft[index].file_len); ++cur_pos)
                mem_area[mem_index++] = oft[index].block[i++];
            count -= i;
        }
        oft[index].cur_pos = cur_pos;
    }

    for(int i=0; i<mem_index; ++i)
        fprintf(out_file, "%c",mem_area[i]);
    fprintf(out_file, "\n");
    return mem_index;
}

int user_write(int index, char* mem_buff, int count)//user to FS
{
    int mem_index=0;
    int cur_pos = oft[index].cur_pos,file_des = oft[index].des_index;
    int b_i=get_i(file_des), b_j= get_j(file_des);
    int cur_block_order = get_block(cur_pos);
    //printf("\ncur_pos: %d  cur_block_order: %d", cur_pos, cur_block_order);
    int end_block_order = get_block(cur_pos + count);
    //printf("cur_pos: %d  cur_block_order: %d\n", (cur_pos+count), end_block_order);
    //fprintf(out_file,"oft[index].block: %s\n", oft[index].block);
    if (cur_block_order == end_block_order)
    {
        
        while( mem_index <count && cur_pos < 192)
            oft[index].block[cur_pos++%64]=mem_buff[mem_index++];
        int cur_block_index= char_arr_to_int(b_i, b_j +4*(cur_block_order));
        oft[index].cur_pos = cur_pos;
        write_block(cur_block_index, index);
        //print_block(cur_block_index);
        if (cur_pos > oft[index].file_len)
            oft[index].file_len = cur_pos;
        int_to_char_arr(b_i, b_j, oft[index].file_len);
    }
    else
    {
        int left_over = 64 - cur_pos %64;
        while( mem_index<left_over)
            oft[index].block[cur_pos++%64] =mem_buff[mem_index++];
        int block= char_arr_to_int(b_i , (b_j+4*cur_block_order));
        write_block(block, index);
        //print_block(block);
        count -= mem_index;
        while(count >0 && cur_pos < 192)//>=0
        {
            int next_block= char_arr_to_int(b_i, b_j +4*(++cur_block_order));
            if(next_block == -1){
                if((next_block= find_empty_block()) == -1)
                    return -1;
                else
                    int_to_char_arr(b_i, b_j + 4*cur_block_order, next_block);
                //printf("The next block is: %d\n", next_block);
            }
            read_block(next_block, index);
            int i =0;
            for(; i < count && i<64 && (cur_pos < 192); ++cur_pos)
                oft[index].block[i++]= mem_buff[mem_index++];
            write_block(next_block, index);
            //print_block(next_block);
            count -= i;
        }
        oft[index].cur_pos = cur_pos;
        if (cur_pos > oft[index].file_len)
            oft[index].file_len = cur_pos;
        int_to_char_arr(b_i, b_j, oft[index].file_len);
    }
    fprintf(out_file, "%d bytes written\n", mem_index);
    return mem_index;
}


int my_open(char* file_name)//return OFT index
{
    int file_des = find_file_in_directory(file_name,0);
    //printf("file des:  %d\n", file_des);
    if(file_des != -1)
    {
        int oft_index =0;
        for(int i =1; i < 4; ++i)
        {
            if(oft[i].des_index == -1)
            {
                oft_index = i;
                break;
            }
        }
        if(oft_index == 0)
        {
            fprintf(out_file, "error\n");
            return -1;
        }
        int i = get_i(file_des), j = get_j(file_des);
        oft[oft_index].des_index = file_des;
        oft[oft_index].cur_pos = 0;
        oft[oft_index].file_len = char_arr_to_int(i, j);
        int first_block = char_arr_to_int(i, j+4);
        if(first_block == -1)
        {
            if((first_block = find_empty_block()) != -1)
                int_to_char_arr(i, j+4, first_block);
            else
            {
                fprintf(out_file,"error\n");
                return -1;
            }
        }
        read_block(first_block, oft_index);//oft[oft_index].block);
        fprintf(out_file,"%s opened %d\n", file_name,oft_index );
        return oft_index;
    }
    fprintf(out_file,"error\n");
    return -1;
}



void init_by_file(FILE* input_file)
{
    clear_oft_table();
    char ch;
    int i=0, j=0;
    
    while(((ch = fgetc(input_file)) != EOF) && i <65)
    {
        if(j ==64)
        {
            ++i;
            j=0;
        }
        if(ch == ' ')
            ldisk[i][j] = '\0';
        else
            ldisk[i][j++] = ch;
    }
    
    int block_index = char_arr_to_int(1, 4);
    read_block(block_index, 0);
    oft[0].cur_pos = 1;
    oft[0].des_index = 0;
    oft[0].file_len = char_arr_to_int(1, 0);
    fprintf(out_file,"disk restored\n");
}



int close(int index)
{
    int cur_block_order = get_block(oft[index].cur_pos);
    int file_des = oft[index].des_index;
    int cur_block= char_arr_to_int(get_i(file_des), get_j(file_des) +4*cur_block_order);
    write_block(cur_block, index);
    int_to_char_arr(get_i(file_des), get_j(file_des), oft[index].file_len);
    oft[index].des_index = -1;
    fprintf(out_file,"%d closed\n", index);
    return 1;
}



int my_lseek(int index, int pos)///////need to rewrite
{
    if(pos <= oft[index].file_len)
    {
        int cur_block_order = get_block(oft[index].cur_pos);
        int new_block_order = get_block(pos);
        if (cur_block_order != new_block_order)
        {
            int i =get_i(oft[index].des_index), j = get_j(oft[index].des_index);
            int cur_block= char_arr_to_int(i, j+ 4*cur_block_order);
            int new_block=char_arr_to_int(i, j+ 4*new_block_order);
            write_block(cur_block, index);
            read_block(new_block, index);
        }
        oft[index].cur_pos = pos;
        fprintf(out_file,"position is %d\n", oft[index].cur_pos);
        return pos;
    }
    else
    {
        fprintf(out_file, "error\n");
        return -1;
    }
}
void save(char* file_name)
{
    FILE* sv_file= fopen(file_name,"w+");
    if(sv_file != NULL)
    {
        for(int i=0; i<64; ++i)
        {
            for(int j=0; j<64; ++j)
            {
                if(ldisk[i][j] != '\0')
                    fprintf(sv_file,"%c", ldisk[i][j]);
                else
                    fprintf(sv_file, "%c",' ');
            }
        }
        fprintf(out_file, "disk saved\n");
    }
    else
        fprintf(out_file, "error\n");
}
void directory()
{
    int block=0;
    write_block(char_arr_to_int(1,4*oft[0].cur_pos),0);
    int num_of_file = char_arr_to_int(1, 0);
    for(int order=1, count=0; order<=3 && count < num_of_file; ++order)
    {
        if( (block = char_arr_to_int(1, 4*order)) != -1)
        {
            read_block(block, 0);
            oft[0].cur_pos = order;
            for(int i=0; i<8;++i)
                if(char_arr_to_int_oft(8*i+4) != -1)
                {
                    fprintf(out_file, "%c%c%c   ",oft[0].block[8*i],oft[0].block[8*i+1],oft[0].block[8*i+2]);
                    ++count;
                }
            fprintf(out_file, "\n");
        }
    }
    
}
void ini_ldisk()
{
    for(int i=0; i<64; ++i)
        for(int j=0; j<64; ++j)
            ldisk[i][j] = '\0';
}

void ini_everything()
{
    ini_bitmap();
    ini_ldisk();
    build_file_descriptor();
    ini_directory();
    for(int i=1; i<4; ++i)
        oft[i].des_index = -1;
    fprintf(out_file, "disk initialized\n");
}


void ini_and_clear_mem_buff()
{
    for(int i=0; i<200; ++i)
        mem_buff[i] = '\0';
}

void process_command(char* command)
{
    unsigned long last = strlen(command);
    if(command[last-1] == '\n')
        command[last-1] = '\0';
    char cmd[5][50];
    for(int i=0; i<5; ++i)
        for(int j=0; j<50; ++j)
            cmd[i][j] = '\0';
    char* token = strtok(command, " ");
    for(int i=0;token != NULL && i<10; )
    {
        strcpy(cmd[i++], token);
        token = strtok(NULL, " ");
    }
    if(strcmp(cmd[0],"in") ==0)
    {
        clear_oft_table();
        if( cmd[1][0] != '\0')
        {
            FILE *input_file = fopen(cmd[1],"r+");
            if(input_file != NULL)
                init_by_file(input_file);
            else
                ini_everything();
        }
        else
            ini_everything();
    }
    else if (strcmp(cmd[0],"cr") ==0)
    {
        if(cmd[1] == NULL || strlen(cmd[1]) >3)
            fprintf(out_file,"error\n");
        else
            create(cmd[1]);
    }
    else if(strcmp(cmd[0],"op") ==0)
    {
        if(strlen(cmd[1]) >3)
            fprintf(out_file, "error\n");
        else
            my_open(cmd[1]);
    }
    else if(strcmp(cmd[0],"rd") ==0)
    {
        if((atoi(cmd[1]) <1) || (atoi(cmd[1]) >=4) || (atoi(cmd[2]) <=0)||oft[atoi(cmd[1])].des_index == -1)
            fprintf(out_file, "error\n");
        else
        {
            ini_and_clear_mem_area();
            user_read(atoi(cmd[1]), atoi(cmd[2]));
        }
        
    }
    else if(strcmp(cmd[0],"wr") ==0)
    {
        if((atoi(cmd[1]) <1) || (atoi(cmd[1]) >=4) || (atoi(cmd[3]) <=0) || (strlen(cmd[2]) != 1) ||oft[atoi(cmd[1])].des_index == -1)
            fprintf(out_file, "error\n");
        else
        {
            ini_and_clear_mem_buff();
            int count = atoi(cmd[3]);
            char ch= cmd[2][0];
            for(int i=0; i< count; ++i )
                mem_buff[i] = ch;
            user_write(atoi(cmd[1]), mem_buff, count);
        }
    }
    else if(strcmp(cmd[0],"cl") ==0)
    {
        if((atoi(cmd[1]) >0) && (atoi(cmd[1]) <4))
            close(atoi(cmd[1]));
        else
            fprintf(out_file, "error\n");
    }
    else if(strcmp(cmd[0],"sk") ==0)
    {
        int index = atoi(cmd[1]), pos= atoi(cmd[2]);
        if((atoi(cmd[1]) <1) || (atoi(cmd[1]) >=4) || (atoi(cmd[2]) <0)||(atoi(cmd[2]) >192))
            fprintf(out_file, "error\n");
        my_lseek(index , pos);
    }
    else if(strcmp(cmd[0],"dr") ==0)
    {
        directory();
    }
    else if(strcmp(cmd[0],"sv") ==0)
    {
        if(cmd[1][0] == '\0')
            fprintf(out_file, "error\n");
        save(cmd[1]);
    }
    else if(strcmp(cmd[0],"de") ==0)
    {
        if(cmd[1][0] == '\0')
            fprintf(out_file, "error\n");
        destory(cmd[1]);
    }
    else
        fprintf(out_file,"error\n");
}

int main(int argc, const char * argv[]) {
    build_MASK();
    char input_path[100],output_path[100];
    printf("Please enter input file's path:\n");
    fgets(input_path,100,stdin);
    unsigned long last_in= strlen(input_path);
    if(input_path[last_in-1] == '\n')
        input_path[last_in-1] = '\0';
    FILE* in_file = fopen(input_path,"r+");
    
    printf("Please enter output file's path:\n");
    scanf("%s",output_path);
    unsigned long last_out= strlen(output_path);

    if(input_path[last_out-1] == '\n')
        input_path[last_out-1] = '\0';

    out_file = fopen(output_path,"w");
    
    if(in_file != NULL && out_file != NULL)
    {
        char command[100];
        //while( fscanf(in_file,"%s",command) != EOF)
        while( fgets(command,100, in_file) != NULL)
        {
            
            if(strcmp(command, "\n")==0)
                fprintf(out_file, "\n");
            else
                process_command(command);
            for(int i=0; i<100; ++i)
                command[i] = '\0';
        }
    }
    else
        fprintf(out_file, "error\n");
    
    fclose(in_file);
    fclose(out_file);
    return 0;
}


