

static char help[] = "Scatters from  a sequential vector to a parallel vector.\n\
   Does case when each local vector is as long as the entire parallel vector.\n";

#include "petsc.h"
#include "is.h"
#include "vec.h"
#include "sys.h"
#include "options.h"
#include "sysio.h"
#include <math.h>

int main(int argc,char **argv)
{
  int           n = 5, ierr;
  int           numtids,mytid,i,N;
  Scalar        one = 1.0, two = 2.0, three = 3.0, value;
  double        norm;
  Vec           x,y;
  IS            is1,is2;
  VecScatterCtx ctx = 0;

  PetscInitialize(&argc,&argv,(char*)0,(char*)0);
  if (OptionsHasName(0,0,"-help")) fprintf(stderr,"%s",help);
  MPI_Comm_size(MPI_COMM_WORLD,&numtids);
  MPI_Comm_rank(MPI_COMM_WORLD,&mytid);

  /* create two vectors */
  N = numtids*n;
  ierr = VecCreateMPI(MPI_COMM_WORLD,-1,N,&y); CHKERR(ierr);
  ierr = VecCreateSequential(N,&x); CHKERR(ierr);

  /* create two index sets */
  ierr = ISCreateStrideSequential(N,0,1,&is1); CHKERR(ierr);
  ierr = ISCreateStrideSequential(N,0,1,&is2); CHKERR(ierr);

  for ( i=0; i<N; i++ ) {
    value = (Scalar) i;
    ierr = VecSetValues(x,1,&i,&value,InsertValues); CHKERR(ierr);
  }
  ierr = VecBeginAssembly(x); CHKERR(ierr);
  ierr = VecEndAssembly(x); CHKERR(ierr);

  VecView(x,0); printf("----\n");

  ierr = VecScatterBegin(x,is2,y,is1,InsertValues,&ctx); CHKERR(ierr);
  ierr = VecScatterEnd(x,is2,y,is1,InsertValues,&ctx); CHKERR(ierr);
  VecScatterCtxDestroy(ctx);
  
  VecView(y,0);

  ierr = VecDestroy(x);CHKERR(ierr);
  ierr = VecDestroy(y);CHKERR(ierr);
  ierr = ISDestroy(is1);CHKERR(ierr);
  ierr = ISDestroy(is2);CHKERR(ierr);

  PetscFinalize(); 
  return 0;
}
 
