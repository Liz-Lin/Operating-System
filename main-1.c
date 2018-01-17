//
//  main.c
//  Project2
//
//  Created by Liz LIn on 11/8/17.
//  Copyright © 2017 Liz Lin. All rights reserved.
//

#include <stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<fcntl.h>
#include<string.h>
FILE* out_file;
struct TLB
{
    int index;
    int sp;
    int f;
};
int FRAME=512;
int TOTAL_FRAME =1024;
int PM[524288];// 1024 frams(512 int)
struct TLB tlb[4];


unsigned int bitmap[32];
unsigned int MASK[32];
unsigned int NMASK[32];

void build_MASKS()
{
    MASK[0]= 0x80000000;
    for(int i=1; i < 32; ++i)
        MASK[i] = MASK[i-1]>> 1;
    for(int i=0; i < 32; ++i)
        NMASK[i] = ~MASK[i];
}



void set_bit_to_1(int address)
{
    int frame_index = (int)(address/FRAME);
    int i= (int)(frame_index/32), j = (frame_index % 32);
    bitmap[i] = (bitmap[i] | MASK[j]);
}

int find_two_empty_frame_and_set()//for new p
{
    int frame_index =0;
    for(int i=0; i<32; ++i)//32*32
        for(int j=0; j<31; ++j)
            if(  ((bitmap[i] & MASK[j]) ==0 ) && ( (bitmap[i] & MASK[j+1]) ==0 ) )
            {
                frame_index = 32*i +j;
                bitmap[i] = (bitmap[i] | MASK[j]);
                bitmap[i] = (bitmap[i] | MASK[j+1]);
                return frame_index;
            }
    
    return -1;
}

int find_empty_frame_and_set()//for new f
{
    int frame_index =0;
    for(int i=0; i<32; ++i)//32*32
        for(int j=0; j<32; ++j)
            if((bitmap[i] & MASK[j]) ==0)
            {
                frame_index = 32*i +j;
                bitmap[i] = (bitmap[i] | MASK[j]);
                return frame_index;
            }
    
    return -1;
}
////////////////////////////////////////////////

void build_segment_table(int s, int p)//check
{
    PM[s] = p;
    if(p > 1)
    {
        set_bit_to_1(p);
        set_bit_to_1(p+FRAME);
    }
}

void build_page_table(int s, int p, int f)//check
{
    PM[PM[s]+p] = f;
    if(f > 0)
        set_bit_to_1(f);
}

void initialize_physical_memory(FILE* in)//check
{
    int s=0, p=0, f=0;

    while(fscanf(in, "%d", &s) )
    {
        fscanf(in, "%d", &p);
        build_segment_table(s, p);
        if(fgetc(in) == '\n')
            break;
    }
    while(fscanf(in, "%d", &p))
    {
        fscanf(in, "%d", &s);
        fscanf(in, "%d", &f);
        build_page_table(s,p,f);
        
        if(fgetc(in) == EOF)
            break;
    }
    

}


int get_s(int va)
{
    int s=0;
    s = va & 0xFF80000;
    s = s >> 19;
    return s;
}

int get_p(int va)
{
    int p=0;
    p = va & 0x7FE00;
    p = p >>9;
    return p;
}

int get_w(int va)
{
    int w=0;
    w = va & 0x1FF;
    return w;
}

int get_sp(int va)//?
{
    int sp=0;
    sp = va>>9;
    return sp;
}

int my_read_without_TLB(int va, char ch)
{
    if(ch != ' ')
        fprintf(out_file, "%c ", ch);
    int s=get_s(va), p= get_p(va), w = get_w(va);
    if(PM[s] ==-1 || PM[PM[s]+p] ==-1)
    {
        fprintf(out_file, "pf ");
        return -1;
    }
    else
    {
        if(PM[s] == 0 || PM[PM[s] + p]== 0)
        {
            fprintf(out_file, "err ");
            return -1;
        }
        else
        {
            int PA = PM[PM[s]+p] +w;
            fprintf(out_file, "%d ",PA );
            return PM[PM[s]+p];
        }
    }
    
}

int my_write_without_TLB(int va, char ch)
{
    if(ch != ' ')
        fprintf(out_file, "%c ", ch);
    int s=get_s(va), p= get_p(va), w = get_w(va);
    if(PM[s] == -1 || PM[PM[s]+p] == -1)
    {
        fprintf(out_file, "pf ");
        return -1;
    }
    else
    {
        if(PM[s] == 0 )
        {
            int new_page_frame = find_two_empty_frame_and_set();
            PM[s] = FRAME * new_page_frame;
        }
        if( PM[PM[s]+p] == 0)
        {
            int new_data_frame = find_empty_frame_and_set();
            PM[PM[s]+p] = FRAME * new_data_frame;
        }
        int PA = PM[PM[s]+p] +w;
        fprintf(out_file, "%d ",PA );
        return PA;
    }
    return -1;
}

void TLB_look_up_read(int va)
{
    //check if file already opened in TLB!!!!!!!!!!^^^^^^^^^^^^^&&&&&&&&&
    int va_sp= va>>9, w = get_w(va);
    for(int i=0; i<4; ++i)
    {
        if(tlb[i].sp == va_sp)
        {
            fprintf(out_file, "h %d ",(tlb[i].f+w) );
            for(int j=0; j <4; ++j)
                if(tlb[j].index > tlb[i].index)
                    --tlb[j].index;
            tlb[i].index =3;
            return;
        }
    }
    int new_f= my_read_without_TLB(va, 'm');
    if(new_f >= 0)
    {
        for(int i=0; i<4; ++i)
        {
            if(tlb[i].index ==0)
            {
                tlb[i].index=3;
                tlb[i].sp = va_sp;
                tlb[i].f = new_f;
            }
            else
                --tlb[i].index;
        }
    }
}


void TLB_look_up_write(int va)
{
    //check if file already opened in TLB!!!!!!!!!!^^^^^^^^^^^^^&&&&&&&&&
    int va_sp= va>>9, w = get_w(va);
    for(int i=0; i<4; ++i)
    {
        if(tlb[i].sp == va_sp)
        {
            fprintf(out_file, "h %d ",(tlb[i].f+w) );
            for(int j=0; j <4; ++j)
                if(tlb[j].index > tlb[i].index)
                    --tlb[j].index;
            tlb[i].index =3;
            return;
        }
    }
    int new_f= my_write_without_TLB(va, 'm');
    if(new_f >= 0)
    {
        for(int i=0; i<4; ++i)
        {
            if(tlb[i].index == 0)
            {
                tlb[i].index=3;
                tlb[i].sp = va_sp;
                tlb[i].f = new_f;
            }
            else
                --tlb[i].index;
        }
    }
    
}



void running_VM_translation_without_TLB(FILE* in)
{
    int o =0, va=0;
    while(fscanf(in, "%d", &o) != EOF )
    {
        fscanf(in, "%d", &va);
        if(va < 0)
            printf("error: va has to be postive\n");
        else
        {
            if(o == 0)
                my_read_without_TLB(va, ' ');
            else if(o==1)
                my_write_without_TLB(va, ' ');
            else
                printf("error: o is either 0 or 1\n");
        }
        if(fgetc(in) == EOF)
            break;
    }
}
//////////////////////////////////////////////////////////////////////////
void running_VM_translation_with_TLB(FILE* in)
{
    int o =0, va=0;
    while(fscanf(in, "%d", &o) != EOF )
    {
        fscanf(in, "%d", &va);
        if(va < 0)
            fprintf(out_file,"error\n");
        else
        {
            if(o == 0)
                TLB_look_up_read(va);
            else if(o==1)
                TLB_look_up_write(va);
            else
                fprintf(out_file,"error: o is either 0 or 1\n");
        }
        if(fgetc(in) == EOF)
            break;
    }
}
/////////////////////////////////////////////////////////////////////////////////////////////
void ini_bitmap()
{
    bitmap[0] = MASK[0];//first frame saved for seg table
    for(int i=1; i<32; ++i)
        bitmap[i] = 0;
}

void ini_PM()
{
    for(int i=0; i<TOTAL_FRAME; ++i)
        for(int j=0; j<FRAME; ++j)
            PM[i*FRAME +j] = 0;
}

void ini_TLB()
{
    for(int i=0; i<4; ++i)
    {
        tlb[i].index = i;
        tlb[i].sp =-1;
    }
}

void ini_everything()
{
    build_MASKS();
    ini_bitmap();
    ini_PM();
    ini_TLB();
}

//////////////////////////////////////////////////////////////////////////
//run without TLB
void without_TLB(char* input1_path, char* input2_path)
{
    char output_path[100];
    printf("Please enter first output file's path:\n");
    scanf("%s",output_path);
    
    out_file = fopen(output_path, "w+");
    
    FILE* in1 = fopen(input1_path,"r+");
    if(in1 != NULL )
        initialize_physical_memory(in1);
    else
        fprintf(out_file,"error can't open %s\n", input1_path);
    fclose(in1);
    
    FILE* in2 = fopen(input2_path,"r+");
    if(in2 != NULL && out_file != NULL)
    {
        running_VM_translation_without_TLB(in2);
    }
    else
        fprintf(out_file,"error can't open %s %s\n", input2_path,output_path);
    fclose(in2);
    fclose(out_file);
}
//////////////////////////////////////////////////////////////////////////
//run with TLB
void with_TLB(char* input1_path, char* input2_path)
{
    char output_path[100];
    printf("Please enter second output file's path:\n");
    scanf("%s",output_path);
    
    out_file = fopen(output_path, "w+");
    
    FILE* in1 = fopen(input1_path,"r+");
    if(in1 != NULL )
        initialize_physical_memory(in1);
    else
        fprintf(out_file,"error can't open %s\n", input1_path);
    fclose(in1);
    
    FILE* in2 = fopen(input2_path,"r+");
    if(in2 != NULL && out_file != NULL)
    {
        running_VM_translation_with_TLB(in2);
    }
    else
        fprintf(out_file,"error can't open %s %s\n", input2_path,output_path);
    fclose(in2);
    fclose(out_file);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

int main(int argc, const char * argv[]) {

    char input1_path[100],input2_path[100];
    printf("Please enter first input file's path:\n");
    scanf("%s",input1_path);

    
    printf("Please enter second input file's path:\n");
    scanf("%s",input2_path);
    
    
    ini_everything();
    without_TLB(input1_path, input2_path);
    
    ini_everything();
    with_TLB(input1_path, input2_path);
    return 0;
}
/*
 file path:
 
 input1:/Volumes/LIZ/input1.txt
 input2:/Volumes/LIZ/input2.txt
 out1: /Volumes/LIZ/822267901.txt
 out2: /Volumes/LIZ/822267902.txt
 
 
 /Users/lizhenlin/Desktop/input0.txt
 /Users/lizhenlin/Desktop/case
 /Users/lizhenlin/Desktop/
 */
