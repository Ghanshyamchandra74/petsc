#include <../src/ksp/ksp/utils/lmvm/lmvm.h> /*I "petscksp.h" I*/

/*
  Limited-memory Symmetric-Rank-1 method for approximating both 
  the forward product and inverse application of a Jacobian.
*/

typedef struct {
  Vec *P;
  Vec work;
  PetscBool allocated;
} Mat_LSR1;

/*------------------------------------------------------------*/

/*
  The solution method is adapted from Algorithm 8 of Erway and Marcia 
  "On Solving Large-Scale Limited-Memory Quasi-Newton Equations" 
  (https://arxiv.org/abs/1510.06378).

  dX <- J0^{-1} * F

  for i = 0,1,2,...,k
    P[i] <- J0^{-1} * Y[i]
    for j = 0,1,2,...,i-1
      W <- S[j] - P[i]
      zeta = (W^T Y[i]) / (W^T Y[j])
      P[i] <- P[i] + (zeta * W)
    end
    W <- S[i] - P[i]
    zeta = (W^T F) / (W^T Y[i])
    dX <- dX + (zeta * W)
  end
*/
static PetscErrorCode MatSolve_LMVMSR1(Mat B, Vec F, Vec dX)
{
  Mat_LMVM          *lmvm = (Mat_LMVM*)B->data;
  Mat_LSR1          *lsr1 = (Mat_LSR1*)lmvm->ctx;
  PetscErrorCode    ierr;
  PetscInt          i, j;
  PetscReal         ptf, pty[lmvm->k+1];
  
  PetscFunctionBegin;
  PetscValidHeaderSpecific(F, VEC_CLASSID, 2);
  PetscValidHeaderSpecific(dX, VEC_CLASSID, 3);
  VecCheckSameSize(F, 2, dX, 3);
  VecCheckMatCompatible(B, dX, 3, F, 2);
  
  /* Invert the initial Jacobian onto F (or apply scaling) */
  ierr = MatLMVMApplyJ0Inv(B, F, dX);CHKERRQ(ierr);
  /* Start outer loop */
  for (i = 0; i <= lmvm->k; ++i) {
    /* Invert the initial Jacobian onto Y[i] (or apply scaling) */
    ierr = MatLMVMApplyJ0Inv(B, lmvm->Y[i], lsr1->P[i]);CHKERRQ(ierr);
    ierr = VecAXPBY(lsr1->P[i], 1.0, -1.0, lmvm->S[i]);CHKERRQ(ierr);
    /* Start the P[i] computation which involves an inner loop */
    for (j = 0; j <= i-1; ++j) {
      ierr = VecDot(lsr1->P[j], lmvm->Y[i], &ptf);CHKERRQ(ierr);
      ierr = VecAXPY(lsr1->P[i], -ptf/pty[i], lsr1->P[j]);CHKERRQ(ierr);
    }
    /* Accumulate the summation term */
    ierr = VecDotBegin(lsr1->P[i], lmvm->Y[i], &pty[i]);CHKERRQ(ierr);
    ierr = VecDotBegin(lsr1->P[i], F, &ptf);CHKERRQ(ierr);
    ierr = VecDotEnd(lsr1->P[i], lmvm->Y[i], &pty[i]);CHKERRQ(ierr);
    ierr = VecDotEnd(lsr1->P[i], F, &ptf);CHKERRQ(ierr);
    ierr = VecAXPY(dX, ptf/pty[i], lsr1->P[i]);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

/*------------------------------------------------------------*/

/*
  The forward product is the matrix-free implementation of 
  Equation (6.24) in Nocedal and Wright "Numerical Optimization" 
  2nd edition, pg 144.
  
  Note that the structure of the forward product is identical to 
  the solution, with S and Y exchanging roles.

  Z <- J0 * X

  for i = 0,1,2,...,k
    P[i] <- J0 * S[i]
    for j = 0,1,2,...,i-1
      W <- Y[j] - P[j]
      zeta = (W^T S[i]) / (W^T Y[j])
      P[i] <- P[i] + (zeta * W)
    end
    W <- Y[i] - P[i]
    zeta = (W^T X) / (W^T Y[i])
    Z <- Z + (zeta * W)
  end
*/
static PetscErrorCode MatMult_LMVMSR1(Mat B, Vec X, Vec Z)
{
  Mat_LMVM          *lmvm = (Mat_LMVM*)B->data;
  Mat_LSR1          *lsr1 = (Mat_LSR1*)lmvm->ctx;
  PetscErrorCode    ierr;
  PetscInt          i, j;
  PetscReal         ptx, pts[lmvm->k+1];
  
  PetscFunctionBegin;
  ierr = MatLMVMApplyJ0Fwd(B, X, Z);CHKERRQ(ierr);
  for (i = 0; i <= lmvm->k; ++i) {
    ierr = MatLMVMApplyJ0Fwd(B, lmvm->S[i], lsr1->P[i]);CHKERRQ(ierr);
    ierr = VecAXPBY(lsr1->P[i], 1.0, -1.0, lmvm->Y[i]);CHKERRQ(ierr);
    for (j = 0; j <= i-1; ++j) {
      ierr = VecDot(lsr1->P[j], lmvm->S[i], &ptx);CHKERRQ(ierr);
      ierr = VecAXPY(lsr1->P[i], -ptx/pts[j], lsr1->P[j]);CHKERRQ(ierr);
    }
    ierr = VecDotBegin(lsr1->P[i], lmvm->S[i], &pts[i]);CHKERRQ(ierr);
    ierr = VecDotBegin(lsr1->P[i], X, &ptx);CHKERRQ(ierr);
    ierr = VecDotEnd(lsr1->P[i], lmvm->S[i], &pts[i]);CHKERRQ(ierr);
    ierr = VecDotEnd(lsr1->P[i], X, &ptx);CHKERRQ(ierr);
    ierr = VecAXPY(Z, ptx/pts[i], lsr1->P[i]);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

/*------------------------------------------------------------*/

static PetscErrorCode MatUpdate_LMVMSR1(Mat B, Vec X, Vec F)
{
  Mat_LMVM          *lmvm = (Mat_LMVM*)B->data;
  Mat_LSR1          *lsr1 = (Mat_LSR1*)lmvm->ctx;
  PetscErrorCode    ierr;
  PetscInt          i, j;
  PetscReal         ptx, pts[lmvm->k+1], worktsnew, snorm, pnorm;

  PetscFunctionBegin;
  if (lmvm->m == 0) PetscFunctionReturn(0);
  if (lmvm->prev_set) {
    /* Compute the new (S = X - Xprev) and (Y = F - Fprev) vectors */
    ierr = VecAXPBY(lmvm->Xprev, 1.0, -1.0, X);CHKERRQ(ierr);
    ierr = VecAXPBY(lmvm->Fprev, 1.0, -1.0, F);CHKERRQ(ierr);
    /* Test if the updates can be accepted */
    for (i = 0; i <= lmvm->k; ++i) {
      ierr = MatLMVMApplyJ0Fwd(B, lmvm->S[i], lsr1->P[i]);CHKERRQ(ierr);
      ierr = VecAXPBY(lsr1->P[i], 1.0, -1.0, lmvm->Y[i]);CHKERRQ(ierr);
      for (j = 0; j <= i-1; ++j) {
        ierr = VecDot(lsr1->P[j], lmvm->S[i], &ptx);CHKERRQ(ierr);
        ierr = VecAXPY(lsr1->P[i], -ptx/pts[j], lsr1->P[j]);CHKERRQ(ierr);
      }
      ierr = VecDot(lsr1->P[i], lmvm->S[i], &pts[i]);CHKERRQ(ierr);
    }
    ierr = MatLMVMApplyJ0Fwd(B, lmvm->Xprev, lsr1->work);CHKERRQ(ierr);
    ierr = VecAXPBY(lsr1->work, 1.0, -1.0, lmvm->Fprev);CHKERRQ(ierr);
    for (j = 0; j <= lmvm->k; ++j) {
      ierr = VecDot(lsr1->P[j], lmvm->Xprev, &ptx);CHKERRQ(ierr);
      ierr = VecAXPY(lsr1->work, -ptx/pts[j], lsr1->P[j]);CHKERRQ(ierr);
    }
    ierr = VecDot(lsr1->work, lmvm->Xprev, &worktsnew);CHKERRQ(ierr);
    ierr = VecNorm(lmvm->Xprev, NORM_2, &snorm);CHKERRQ(ierr);
    ierr = VecNorm(lsr1->work, NORM_2, &pnorm);CHKERRQ(ierr);
    if (PetscAbsReal(worktsnew) >= lmvm->eps * snorm * pnorm) {
      /* Update is good, accept it */
      ierr = MatUpdateKernel_LMVM(B, lmvm->Xprev, lmvm->Fprev);CHKERRQ(ierr);
    } else {
      /* Update is bad, skip it */
      ++lmvm->nrejects;
    }
  }

  /* Save the solution and function to be used in the next update */
  ierr = VecCopy(X, lmvm->Xprev);CHKERRQ(ierr);
  ierr = VecCopy(F, lmvm->Fprev);CHKERRQ(ierr);
  lmvm->prev_set = PETSC_TRUE;
  PetscFunctionReturn(0);
}

/*------------------------------------------------------------*/

static PetscErrorCode MatReset_LMVMSR1(Mat B, PetscBool destructive)
{
  Mat_LMVM          *lmvm = (Mat_LMVM*)B->data;
  Mat_LSR1          *lsr1 = (Mat_LSR1*)lmvm->ctx;
  PetscErrorCode    ierr;
  
  PetscFunctionBegin;
  if (destructive && lsr1->allocated && lmvm->m > 0) {
    ierr = VecDestroy(&lsr1->work);CHKERRQ(ierr);
    ierr = VecDestroyVecs(lmvm->m, &lsr1->P);CHKERRQ(ierr);
    lsr1->allocated = PETSC_FALSE;
  }
  ierr = MatReset_LMVM(B, destructive);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/*------------------------------------------------------------*/

static PetscErrorCode MatAllocate_LMVMSR1(Mat B, Vec X, Vec F)
{
  Mat_LMVM          *lmvm = (Mat_LMVM*)B->data;
  Mat_LSR1          *lsr1 = (Mat_LSR1*)lmvm->ctx;
  PetscErrorCode    ierr;
  
  PetscFunctionBegin;
  ierr = MatAllocate_LMVM(B, X, F);CHKERRQ(ierr);
  if (!lsr1->allocated && lmvm->m > 0) {
    ierr = VecDuplicate(X, &lsr1->work);CHKERRQ(ierr);
    ierr = VecDuplicateVecs(X, lmvm->m, &lsr1->P);CHKERRQ(ierr);
    lsr1->allocated = PETSC_TRUE;
  }
  PetscFunctionReturn(0);
}

/*------------------------------------------------------------*/

static PetscErrorCode MatDestroy_LMVMSR1(Mat B)
{
  Mat_LMVM          *lmvm = (Mat_LMVM*)B->data;
  Mat_LSR1          *lsr1 = (Mat_LSR1*)lmvm->ctx;
  PetscErrorCode    ierr;

  PetscFunctionBegin;
  if (lsr1->allocated && lmvm->m > 0) {
    ierr = VecDestroy(&lsr1->work);CHKERRQ(ierr);
    ierr = VecDestroyVecs(lmvm->m, &lsr1->P);CHKERRQ(ierr);
    lsr1->allocated = PETSC_FALSE;
  }
  ierr = PetscFree(lmvm->ctx);CHKERRQ(ierr);
  ierr = MatDestroy_LMVM(B);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/*------------------------------------------------------------*/

static PetscErrorCode MatSetUp_LMVMSR1(Mat B)
{
  Mat_LMVM          *lmvm = (Mat_LMVM*)B->data;
  Mat_LSR1          *lsr1 = (Mat_LSR1*)lmvm->ctx;
  PetscErrorCode    ierr;
  
  PetscFunctionBegin;
  ierr = MatSetUp_LMVM(B);CHKERRQ(ierr);
  if (!lsr1->allocated && lmvm->m > 0) {
    ierr = VecDuplicate(lmvm->Xprev, &lsr1->work);CHKERRQ(ierr);
    ierr = VecDuplicateVecs(lmvm->Xprev, lmvm->m, &lsr1->P);CHKERRQ(ierr);
    lsr1->allocated = PETSC_TRUE;
  }
  PetscFunctionReturn(0);
}

/*------------------------------------------------------------*/

PetscErrorCode MatCreate_LMVMSR1(Mat B)
{
  Mat_LSR1          *lsr1;
  PetscErrorCode    ierr;

  PetscFunctionBegin;
  ierr = MatCreate_LMVM(B);CHKERRQ(ierr);
  ierr = PetscObjectChangeTypeName((PetscObject)B, MATLMVMSR1);CHKERRQ(ierr);
  ierr = MatSetOption(B, MAT_SYMMETRIC, PETSC_TRUE);CHKERRQ(ierr);
  B->ops->solve = MatSolve_LMVMSR1;
  B->ops->setup = MatSetUp_LMVMSR1;
  B->ops->destroy = MatDestroy_LMVMSR1;
  
  Mat_LMVM *lmvm = (Mat_LMVM*)B->data;
  lmvm->square = PETSC_TRUE;
  lmvm->ops->allocate = MatAllocate_LMVMSR1;
  lmvm->ops->reset = MatReset_LMVMSR1;
  lmvm->ops->update = MatUpdate_LMVMSR1;
  lmvm->ops->mult = MatMult_LMVMSR1;
  
  ierr = PetscNewLog(B, &lsr1);CHKERRQ(ierr);
  lmvm->ctx = (void*)lsr1;
  lsr1->allocated = PETSC_FALSE;
  PetscFunctionReturn(0);
}

/*------------------------------------------------------------*/

/*@
   MatCreateLMVMSR1 - Creates a limited-memory Symmetric-Rank-1 approximation
   matrix used for a Jacobian. L-SR1 is symmetric by construction, but is not 
   guaranteed to be positive-definite.
   
   The provided local and global sizes must match the solution and function vectors 
   used with MatLMVMUpdate() and MatSolve(). The resulting L-SR1 matrix will have 
   storage vectors allocated with VecCreateSeq() in serial and VecCreateMPI() in 
   parallel. To use the L-SR1 matrix with other vector types, the matrix must be 
   created using MatCreate() and MatSetType(), followed by MatLMVMAllocate(). 
   This ensures that the internal storage and work vectors are duplicated from the 
   correct type of vector.

   Collective on MPI_Comm

   Input Parameters:
+  comm - MPI communicator, set to PETSC_COMM_SELF
.  n - number of local rows for storage vectors
-  N - global size of the storage vectors

   Output Parameter:
.  B - the matrix

   It is recommended that one use the MatCreate(), MatSetType() and/or MatSetFromOptions()
   paradigm instead of this routine directly.

   Options Database Keys:
.   -mat_lmvm_num_vecs - maximum number of correction vectors (i.e.: updates) stored

   Level: intermediate

.seealso: MatCreate(), MATLMVM, MATLMVMSR1, MatCreateLMVMBFGS(), MatCreateLMVMDFP(), 
          MatCreateLMVMBrdn(), MatCreateLMVMBadBrdn(), MatCreateLMVMSymBrdn()
@*/
PetscErrorCode MatCreateLMVMSR1(MPI_Comm comm, PetscInt n, PetscInt N, Mat *B)
{
  PetscErrorCode    ierr;
  
  PetscFunctionBegin;
  ierr = MatCreate(comm, B);CHKERRQ(ierr);
  ierr = MatSetSizes(*B, n, n, N, N);CHKERRQ(ierr);
  ierr = MatSetType(*B, MATLMVMSR1);CHKERRQ(ierr);
  ierr = MatSetUp(*B);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}