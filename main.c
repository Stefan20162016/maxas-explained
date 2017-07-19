#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

char * int2bin(int i)
{
	  size_t bits = 8;
    char * str = malloc(bits + 1);
    if(!str) return NULL;
    str[bits] = 0;
    unsigned u = *(unsigned *)&i;
    for(; bits--; u >>= 1)
        str[bits] = u & 1 ? '1' : '0';

    return str;
}

void printMatrix2(char * string, int * matrix, int m, int k){
		printf("%s\n",string);
	for(int i=0; i<k; ++i){
		for(int j=0; j<m; ++j){
			printf ("%d ", *(matrix+j+i*k) );
		}
		printf("\n");
	}
	printf("\n");
}

void printMatrix(char * string, int * matrix, int m, int n){
		printf("%s\n",string);
	for(int i=0; i<m; ++i){
		for(int j=0; j<n; ++j){
			printf ("%4d ", *(matrix + i + j*m ) );
		}
		printf("\n");
	}
	printf("\n");
}

void test(){
	void *x = malloc(8);
	char *sh2 = malloc(5);
	char *sh =x;
	 *sh      = 0 ;
	 *(sh +1) = 0;
	 *(sh +2) = 1;
	 *(sh +3) = 0;
	 int *i=x;
	 printf("output: %d\n", *(sh+2));
	 printf("output int: %d\n", *i);
	 *(i+1)=16777215;
	 printf("output int: %d\n", *(i+1));
	 int *j=x;
	 printf("output int: %d\n", (*(j+1)));
}


int main(){
	
	//test();
	//exit(0) ;

	int myCounter=1, myCounter2=0;
	int k = 64;
	int tid; // tid = threadIdx.x
	int bx=0, by=0; // bx = blockId.x; by = blockId.y
	int lda=64, ldb=64, ldc=64; 
	int texA=0, texB=1;
	int *m_A = (int *)malloc(4*k*(lda)); // /4 cause 4xtexture index
	int *m_B = (int *)malloc(4*k*lda);
	int *m_C = (int *)malloc(4*lda*ldb);
	int *shared = (int *)malloc(8192); // loop:8 blockwidth:64 word:4 A:2x B:2x
	// 2x2048 for A 2x2048 for B;A:0-2047:B:2048-4095:A:4096-6143:B:6144-
	for(int i=0; i<k; ++i){
		for(int j=0; j<lda; ++j){
			*(m_A+j+i*lda) = j + i*lda;
		}
	}
	for(int i=0; i<k; ++i){
		for(int j=0; j<ldb; ++j){
			*(m_B+j+i*ldb) = 4096 + j + i*ldb;
		}
	}
	for(int i=0; i<ldb; ++i){
		for(int j=0; j<ldc; ++j){
			*(m_C+j+i*ldc) = 8192 + j + i*ldc;
		}
	}
	for(int i=0; i<2048; i++){
		*(shared + i) = -i - 1000000; // make it visible if shared is not overwritten
	}
	/*printf("matrix A:\n");
	for(int i=0; i<k; ++i){q
		for(int j=0; j<lda; ++j){
			printf ("%d ", *(m_A+j+i*k) );
		}
		printf("\n");
	}
*/
printMatrix("matrix A",m_A,lda,k);
//printMatrix("matrix B",m_B,ldb,k);
//printMatrix("matrix C",m_C,lda,ldb);

	for(tid=0; tid<64; tid++){

		//int tidm32 = tid & -32;
		//printf("tid %d tid&-32 %d\n",tid,tidm32);
		//continue;

	 //if(tid == 32){int x=getc(stdin);}
		int blk = tid >= 32 ? by:bx;
		int ldx = tid >= 32 ? ldb/4:lda/4;
		int tex = tid >= 32 ? texB : texA;
		int tid4 = (tid >> 4) & 3;
		int tid2=(tid>>4) & 1 ;
		int tid15=tid & 15;
		int track0 = blk*64/4 + tid15 + (ldx*tid2); // tex index for 4 at a time
		int track2 = track0 + ldx*2;
		int track4 = track0 + ldx*4;
		int track6 = track0 + ldx*6;
		int end = track0 + (k-8)*ldx;
		int writeS = tid15*4*4 + tid2*64*4;
		writeS += tid >= 32 ? 2048:0;
		int writeSxor = writeS ^ 4096;
		int readAs = ((tid >> 1)&7) << 4; // index in shared bytewise
		int readBs = (((tid&0x30) >> 3) | (tid&1)) << 4 ;
		readBs += 2048;
		//printf("tid %d tid4 %d readAs %d readBs %d\n",tid,tid4,readAs, readBs);
		int j0Ax00=-60, j0By00=-61, j0Ax32=-62, j0By32=-63;
		int j1Ax00=-70, j1By00=-71, j1Ax32=-72, j1By32=-73;

		

	int whileLoopCounter=0;
	while( track0 <= end){
		if (whileLoopCounter == 2){  // the 32 threads loaded 16 in each iteration 32*16=512, so the shared mem is now fully loaded 2x512 from A and 2x512 from B; 2048 units=8192bytes
		//printf("break after 2x while loop %d\n",whileLoopCounter);
		break;
	}
		whileLoopCounter++;
		int * m_ = tid >= 32 ? m_B:m_A;
		//printf("track0: %d\ttrack2: %d\ttrack4: %d\ttrack6: %d\tend %d\ttid: %d\tcount %d\n",track0,track2,track4,track6,end,tid,myCounter++);

		printf("track0: %d\ttrack2: %d\ttrack4: %d\ttrack6: %d\tend %d\ttid: %d\twriteS %d readAB\t%d %d\n",track0,track2,track4,track6,end,tid,writeS,readAs,readBs);
		// load matrix A and B via 4 <4 x texture> loads
		int mN= tid >= 32 ? -1:1; // mult by -10 to see different numbers for matrix A and B
		
		*(m_ + track0*4)  = mN*(track0*4);  *(m_ + track0*4+1)= mN*(track0*4+1); // track0
		*(m_ + track0*4+2)= mN*(track0*4+2); *(m_ + track0*4+3)= mN*(track0*4+3); 

		*(m_ + track2*4)  = mN*(track2*4);  *(m_ + track2*4+1)= mN*(track2*4+1);
		*(m_ + track2*4+2)= mN*(track2*4+2); *(m_ + track2*4+3)= mN*(track2*4+3);

		*(m_ + track4*4)  = mN*(track4*4);  *(m_ + track4*4+1)= mN*(track4*4+1);
		*(m_ + track4*4+2)= mN*(track4*4+2); *(m_ + track4*4+3)= mN*(track4*4+3);

		*(m_ + track6*4)  = mN*(track6*4);  *(m_ + track6*4+1)= mN*(track6*4+1);
		*(m_ + track6*4+2)= mN*(track6*4+2); *(m_ + track6*4+3)= mN*(track6*4+3);

		// store to shared
		
		*(shared + writeS/4 + 0*64)      = *(m_ + track0*4);
		*(shared + writeS/4 + 0*64 + 1)  = *(m_ + track0*4 + 1);
		*(shared + writeS/4 + 0*64 + 2)  = *(m_ + track0*4 + 2);
		*(shared + writeS/4 + 0*64 + 3)  = *(m_ + track0*4 + 3);
		
		*(shared + writeS/4 + 2*64)      = *(m_ + track2*4);
		*(shared + writeS/4 + 2*64 + 1)  = *(m_ + track2*4 + 1);
		*(shared + writeS/4 + 2*64 + 2)  = *(m_ + track2*4 + 2);
		*(shared + writeS/4 + 2*64 + 3)  = *(m_ + track2*4 + 3);

		*(shared + writeS/4 + 4*64)      = *(m_ + track4*4);
		*(shared + writeS/4 + 4*64 + 1)  = *(m_ + track4*4 + 1);
		*(shared + writeS/4 + 4*64 + 2)  = *(m_ + track4*4 + 2);
		*(shared + writeS/4 + 4*64 + 3)  = *(m_ + track4*4 + 3);

		*(shared + writeS/4 + 6*64)      = *(m_ + track6*4);
		*(shared + writeS/4 + 6*64 + 1)  = *(m_ + track6*4 + 1);
		*(shared + writeS/4 + 6*64 + 2)  = *(m_ + track6*4 + 2);
		*(shared + writeS/4 + 6*64 + 3)  = *(m_ + track6*4 + 3);

		track0 += ldx*8; 
		track2 += ldx*8;
		track4 += ldx*8;
		track6 += ldx*8;
		writeS ^= 4096;
		readAs ^= 4096;
		readBs ^= 4096;

/*
	if(tid == 0){
		printf("shared for tid %d:\n", tid);
		int cpos=0;
		for(int i=0;i<64;i++){
			for(int j=0; j<32; j++){
				printf("(%.2d:%.2d) %.4d ",i,j,*(shared + j*64 + i));
				if(*(shared + j*64 + i) >= 0){cpos++;}
			}
			printf("\n");
		}
		printf("cpos: %d\n",cpos);
	}
*/

		//int x=getc(stdin);


} //while loop end
	myCounter=1; // reset

	
} // for loop end

for(tid=0; tid<64; tid++){ // simulated bar.sync run this after all 64 threads are done loading/storing into shared memory

		int blk = tid >= 32 ? by:bx;
		int ldx = tid >= 32 ? ldb/4:lda/4;
		int tex = tid >= 32 ? texB : texA;
		int tid2=(tid>>4) & 1 ;
		int tid15=tid & 15;
		int track0 = blk*64/4 + tid15 + (ldx*tid2); // tex index for 4 at a time
		int track2 = track0 + ldx*2;
		int track4 = track0 + ldx*4;
		int track6 = track0 + ldx*6;
		int end = track0 + (k-8)*ldx;
		int writeS = tid15*4*4 + tid2*64*4;
		writeS += tid >= 32 ? 2048:0;
		int writeSxor = writeS ^ 4096;
		int readAs = ((tid >> 1)&7) << 4; // index in shared bytewise
		int readBs = (((tid&0x30) >> 3) | (tid&1)) << 4 ;
		readBs += 2048;
		int j0Ax00=-60, j0By00=-61, j0Ax32=-62, j0By32=-63;
		int j1Ax00=-70, j1By00=-71, j1Ax32=-72, j1By32=-73;

					
		for(int j=0;j<8;j++){
		int prefetch = (j+1-1)%8; // 
		printf("loop: %d for tid %d prefetch %d\n",j, tid, prefetch);
		if(j&1){
			//j0Ax00 etc are quad-vector register
			j0Ax00 = shared[(readAs + 4*(prefetch*64+ 0))/4] = 1127; // /4 cause byte indexed
			shared[(readAs + 4*(prefetch*64+ 0))/4 + 1] = 1128;
			shared[(readAs + 4*(prefetch*64+ 0))/4 + 2] = 1129;
			shared[(readAs + 4*(prefetch*64+ 0))/4 + 3] = 1130;
			shared[(readAs + 4*(prefetch*64+ 32))/4 + 1] = 2828;
			shared[(readAs + 4*(prefetch*64+ 32))/4 + 2] = 2929;
			shared[(readAs + 4*(prefetch*64+ 32))/4 + 3] = 3030;
			j0Ax32 = shared[(readAs + 4*(prefetch*64+32))/4] = 2727;
			j0By00 = shared[(readBs + 4*(prefetch*64+ 0))/4];
			j0By32 = shared[(readBs + 4*(prefetch*64+32))/4];
			printf("odd j0Ax00 %.3d j0Ax32 %.3d j0By00 %.3d j0By32 %.3d\n",j0Ax00,j0Ax32,j0By00,j0By32);
		} else {
			j1Ax00 = shared[(readAs + 4*(prefetch*64+ 0))/4] = 27;
			shared[(readAs + 4*(prefetch*64+ 0))/4 + 1] = 27;
			shared[(readAs + 4*(prefetch*64+ 0))/4 + 2] = 27;
			shared[(readAs + 4*(prefetch*64+ 0))/4 + 3] = 27;
			shared[(readAs + 4*(prefetch*64+ 32))/4 + 1] = 27;
			shared[(readAs + 4*(prefetch*64+ 32))/4 + 2] = 27;
			shared[(readAs + 4*(prefetch*64+ 32))/4 + 3] = 27;
			j1Ax32 = shared[(readAs + 4*(prefetch*64+32))/4] = 27;
			j1By00 = shared[(readBs + 4*(prefetch*64+ 0))/4];
			j1By32 = shared[(readBs + 4*(prefetch*64+32))/4];
			printf("eve j1Ax00 %.3d j1Ax32 %.3d j1By00 %.3d j1By32 %.3d\n",j1Ax00,j1Ax32,j1By00,j1By32);
		}
		
		
} // for prefetch loop 
printf("\n");
		
		readAs ^= 4096;
		readBs ^= 4096;


		int tid31 = tid & 31;
		int tid32 = tid & 32;
		readAs &= 0x7ff; // remove high bits
		readBs &= 0x7ff;
		int writeCs = (readBs/4)*64 + readAs;
		int readCs = ((tid32 << 3) + tid31) << 2;
		int ldc4 = ldc*4;
		int cx = bx*64 + tid31;
		int cy = by*64 + (tid32 >> 1);
		printf("tid %2d writeCs %4d readCs %4d tid31 %d tid32 %d\n",tid,writeCs,readCs,tid31,cy);

} //for tid loop

// to print shared loading add a "break;" at the bottom of the while loop because shared is reloaded every time
// shared: loop:8 blockwidth:64 word:4 A:2x B:2x
// 2x2048 for A 2x2048 for B;A:0-2047:B:2048-4095:A:4096-6143:B:6144-
printf("shared:\n");
int cpos=0;
for(int i=0;i<64;i++){
	for(int j=0; j<32; j++){
		//printf("(%d:%d) %d ",j,i,*(shared + j + i*64));
		printf("(%.2d:%.2d) %.4d ",i,j,*(shared + j*64 + i));
		if(*(shared + j*64 + i) >= 0){cpos++;}
	}
	printf("\n");
}
printf("cpos: %d\n",cpos);

//printMatrix("matrix A_postloop",m_A,lda,k);
//printMatrix("matrix B_postloop",m_B,ldb,k);

}
