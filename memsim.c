//02182016
// Page replacement algorithm
//Chengbin Hu
#include <stdint.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <time.h>
//define value bit dirty bit and reference bit as 20 21 and 22
#define V_BIT 20
#define D_BIT 21
#define R_BIT 22
#define MAXN 4294967295
struct list_el {
   uint32_t val;
   struct list_el * next;
   struct list_el * prev;
};

typedef struct list_el item;

struct trace_intr {
    uint32_t address;
    char mode;
};
/*whether page is dirty*/
int is_d(uint32_t *pt, uint32_t page) {
    int d;
    d = pt[page] & 1<<D_BIT;
    return d;
}


void clear(uint32_t *pt, uint32_t p_n){
	uint32_t m = 7<<V_BIT;
	m = ~m;
	pt[p_n] = pt[p_n] & m;
}



void activate(uint32_t * pt, uint32_t p_n, uint32_t frame, char mode){
    pt[p_n] = frame;     
    pt[p_n] = pt[p_n] | 1<<V_BIT;         
    pt[p_n] = pt[p_n] | 1<<R_BIT;    
    if(mode == 'W'){                    
        pt[p_n] = pt[p_n] | 1<<D_BIT;
    }
}
void debuglog(char *model,char*string,int arg)
{
   if(strcmp(model, "debug") == 0)
       printf(string,arg);
}
void vms(struct trace_intr *lines, uint32_t count, uint32_t *pt, int nframes, char *model)
{
    uint32_t p_n;
    uint32_t *r;
    r = malloc(nframes * sizeof(uint32_t));
    uint32_t i;
    int v_p = 0, t = 0, w = 0, f = 0, x = 0;
    int *refer = malloc(nframes*sizeof(int));//reference list
    for(i = 0; i < nframes; i++)
    {
        refer[i]=0;
    }
    for(i = 0; i < count; i++)
    {
        p_n = lines[i].address>>12;//get pagenumber from address first 20bits
        if((pt[p_n] & 1<<V_BIT)==0)//not valid, miss
        {
            f++;// # fetching from disk
            if(v_p < nframes)// aviable frame 
            {
                debuglog(model,"%i. page fault with out replacement valid pages\n",i); 
                r[v_p] = p_n;//refered frame number and 
                activate(pt, p_n, v_p, lines[i].mode);
                v_p++;
            }else// need to replace
            {
                t = x; //pick up the fist in
                x = (x+1)%nframes;//next
		int j=0;// count
		int n1=-1,n2=-1;
		int k = t;
                for(j=0;j<nframes;j++)
		{
		    if((refer[k]==0&&!is_d(pt,r[k]))&&n1==-1)
                    {
			n1 = k;
			t = n1;// not r not m
			break;
		    }
		    if((refer[k]==0&&is_d(pt,r[k]))&&n2==-1)
                	n2 = k;
		    if(n2!=-1) 
			refer[k] = 0;
	            k = (k+1)%nframes;//next
		}
		if(n1==-1&&n2==-1)
		{
	            for(j=0;j<nframes;j++)
		    {
		        if((refer[k]==0&&!is_d(pt,r[k]))&&n1==-1)
                        {
 			    n1 = k;
			    t = n1;// not r not m
			    break;
		        }
		        if((refer[k]==0&&is_d(pt,r[k]))&&n2==-1)
                    	    n2 = k;
	                k = (k+1)%nframes;//next
		    }
                }else if(n1==-1){t=n2;}
                
		x=(t+1)%nframes;
                if(is_d(pt, r[t]))//check ditry
                {
                    w++;// write back to disk
                    debuglog(model, "%i. page fault with replacement valid pages\n",i);
                }else
                { 
                    debuglog(model, "%i. page fault with replacement invalid pages\n",i);
                }
                clear(pt, r[t]);//set old page invalid
                r[t] = p_n;
                activate(pt, p_n, t, lines[i].mode);//valid new page
            }
        }else//valid, hit
        {
            debuglog(model, "%i, page hit \n", i);
            t = 0;
            while(r[t] != p_n)
            {
                t++;
            }
            refer[t] = 1;// refered once
            if(lines[i].mode == 'W'){// write to mem, modify.
                pt[p_n] = pt[p_n] | 1<<D_BIT;
            }
        }
    }//for 
    printf("Total memory frames:   %i\n", nframes);
    printf("Events in trace:       %i\n", i);
    printf("Total disk reads:      %i\n", f);
    printf("Total disk writes:     %i\n", w); 
}
void lru(struct trace_intr *lines, uint32_t count, uint32_t *pt, int nframes, char *model)
{
    uint32_t p_n;//page number
    uint32_t *r;//the memory we have
    r = malloc(nframes * sizeof(uint32_t));//phsical memory each frame contain a page table entry.
    uint32_t i;//iteration
    int v_p;//valid page
    int t=0;//the page to evict
    int w;//writes
    int f;//pagefaults
    int x;//index to locate frames
    uint32_t a,b;
    item * curr, *head, *tem, *tail;
    b = 0;
    v_p = 0;
    w = 0;
    f = 0;
    x = 0;
    for(i = 0; i < count; i++)
    {
    p_n = lines[i].address>>12;//get pagenumber from address first 20bits
    if((pt[p_n] & 1<<V_BIT)==0)
        {
        f++;
        if(v_p < nframes)
            {
            if(strcmp(model, "debug") == 0)printf("%i. page fault with out replacement valid pages\n",i); 
            r[v_p] = p_n;
            if(v_p==0){//the first page in the track list
                head = (item *)malloc(sizeof(item));
                head->val=v_p;
                head->prev = NULL;
                head->next = NULL;
                tail = head;
            }else{
                curr = (item *)malloc(sizeof(item));//follow pages in the list
                curr->val = v_p;
                curr->prev  = tail;
                tail->next =curr;
                curr->next = NULL;
                tail = curr;                
            }
            activate(pt, p_n, v_p, lines[i].mode);
            v_p++;
            }else
            {
            if(nframes>1)
            {
                t = head->val;               //pick up the oldest one which is the head of the linked list
                /*because head frame is right now the most recent read. we move it to tail.*/
                tem = head;
                head = head->next;//move the head to the next oldest one
                head->prev = NULL;
                tem->prev = tail;//move previous head to tail
                tem->next = NULL;
                tail->next = tem;
                tail = tem;
            }
            if(is_d(pt, r[t])){
                w++;
                if(strcmp(model, "debug") == 0)printf("%i. page fault with replacement valid pages\n",i);
                }else{ 

                if(strcmp(model, "debug") == 0)printf("%i. page fault with replacement invalid pages\n",i);
                }
                clear(pt, r[t]);
                r[t] = p_n;
                activate(pt, p_n, t, lines[i].mode); 
            }
         }else{
                if(strcmp(model, "debug") == 0)printf("%i, page hit \n", i);
                for(a=0;a<nframes;a++){//find the hit page frame number
                    if (r[a]==p_n){
                        b=a;
                        break;
                      }
                    }
                    /*find the hit frame number in the linked list*/
                tem = head;
                while(1){
                    if(tem->val == b){//printf("link list hit val:%d, \n",tem->val);
                        break;}
                    tem = tem->next;
                }
                if(tem->val == b){//move the hit page tem to the tail of lookup list
                    if(tem->prev!=NULL&&tem->next!=NULL){//if hit page is in the middle

                        tem->prev->next = tem->next;//delete from the middle of the list
                        tem->next->prev = tem->prev;
                    
                        tem->prev = tail;//move hit page to tail
                        tem->next = NULL;
                        tail->next = tem;
                        tail = tem;

                    }else if(tem->prev==NULL){//if hit page is the head of look up
                        if(nframes>1){
                        head = head->next;//move the head to the next oldest one
                        head->prev = NULL;
                        tem->prev = tail;//move previous head to tail
                        tem->next = NULL;
                        tail->next = tem;
                        tail = tem;}
                    }//if hit is in the tail then do nothing
                }else{
                    fprintf(stderr, "ERROR! a page hit but cannot find page in memory\n");
                    exit(1);
                }
                pt[p_n] = pt[p_n] | 1<<R_BIT;
                if(lines[i].mode == 'W'){
                    pt[p_n] = pt[p_n] | 1<<D_BIT;
                    }
               }
     }//for 
     printf("Total memory frames:   %i\n", nframes);
     printf("Events in trace:       %i\n", i);
     printf("Total disk reads:      %i\n", f);
     printf("Total disk writes:     %i\n", w);
}



int main(int argc, char *argv[])
{
   if(argc != 5) {
        fprintf(stderr, "ERROR please input four arguments: <tracefile> <nframes> <vms|lru> <debug|quiet> \n");
        exit(1);
    }
    FILE *tfile;
    char *tracefile;
    int nframes;
    char *algorithm;
    unsigned address;
    char rw;
    uint32_t i;
    char *model;
    uint32_t *pagetable; 
    uint32_t page;
    page = 1<<20;//defaule pagesize=4096. 32 bit space. pages=1<<20
    struct trace_intr *traces;
    struct stat buf;
    uint32_t filesize;
    uint32_t tracenums;
    pagetable = malloc(page * sizeof(uint32_t));//pagetable size=pages*4bytes.
    memset(pagetable, 0, page * 4);
    tracefile = argv[1];
    nframes = atoi(argv[2]);
    algorithm = argv[3];
    model = argv[4];
    stat(tracefile, &buf);
    filesize = buf.st_size; 
    tracenums = filesize / 11; 
    tfile = fopen(tracefile, "r");
    if(tfile == NULL) {
        fprintf(stderr, "Unable to open trace file.\n");
        exit(1);
    }
    traces = malloc( tracenums * sizeof(struct trace_intr));
    for(i = 0; i < tracenums; i++){
        fscanf(tfile,"%x %c", &address, &rw);
        traces[i].address = address;
        traces[i].mode = rw;
    }
    
    if(strcmp(algorithm, "vms") == 0){
        vms(traces, tracenums, pagetable, nframes, model);
    } else if(strcmp(algorithm, "lru") == 0) {
    	lru(traces, tracenums, pagetable, nframes, model);
    } else {
    	printf("please indicate the right algorithm<vms|lru>\n");
    }
    fclose(tfile);
    free(traces);
    free(pagetable);
}

