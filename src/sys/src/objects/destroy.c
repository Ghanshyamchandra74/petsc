#ifndef lint
static char vcid[] = "$Id: destroy.c,v 1.20 1996/01/12 17:24:53 balay Exp balay $";
#endif
/*
     Provides utility routines for manulating any type of PETSc object.
*/
#include "petsc.h"  /*I   "petsc.h"    I*/

static int PetscObjectInherit_DefaultCopy(void *in, void **out)
{
  *out = in;
  return 0;
}

/*@C
  PetscObjectInherit - Associate another object with a given PETSc object. 
      This is to provide a limited support for inheritence when using 
      PETSc from C++.

  Input Parameters:
.   obj - the PETSc object
.   ptr - the other object to associate with the PETSc object
.   copy - a function used to copy the other object when the PETSc object 
           is copyed, or PETSC_NULL to indicate the pointer is copied.

@*/
int PetscObjectInherit(PetscObject obj,void *ptr, int (*copy)(void *,void **))
{
  obj->child = ptr;
  if (copy == PETSC_NULL) copy = PetscObjectInherit_DefaultCopy;
  obj->childcopy = copy;
  return 0;
}

/*@C
   PetscObjectDestroy - Destroys any PetscObject, regardless of the type. 
   This routine should seldom be needed.

   Input Parameters:
.  obj - any PETSc object, for example a Vec, Mat or KSP.

.keywords: object, destroy
@*/
int PetscObjectDestroy(PetscObject obj)
{
  if (!obj) SETERRQ(1,"PetscObjectDestroy:Null object");
  if (obj->destroy) return (*obj->destroy)(obj);
  return 0;
}

/*@C
   PetscObjectGetComm - Gets the MPI communicator for any PetscObject, 
   regardless of the type.

   Input Parameter:
.  obj - any PETSc object, for example a Vec, Mat or KSP.

   Output Parameter:
.  comm - the MPI communicator

.keywords: object, get, communicator, MPI
@*/
int PetscObjectGetComm(PetscObject obj,MPI_Comm *comm)
{
  if (!obj) SETERRQ(1,"PetscObjectGetComm:Null object");
  *comm = obj->comm;
  return 0;
}

/*@C
   PetscObjectGetType - Gets the type for any PetscObject, 

   Input Parameter:
.  obj - any PETSc object, for example a Vec, Mat or KSP.

   Output Parameter:
.  type - the type

.keywords: object, get, type
@*/
int PetscObjectGetType(PetscObject obj,int *type)
{
  if (!obj) SETERRQ(1,"PetscObjectGetComm:Null object");
  *type = obj->type;
  return 0;
}

/*@C
   PetscObjectGetCookie - Gets the cookie for any PetscObject, 

   Input Parameter:
.  obj - any PETSc object, for example a Vec, Mat or KSP.

   Output Parameter:
.  cookie - the cookie

.keywords: object, get, cookie
@*/
int PetscObjectGetCookie(PetscObject obj,int *cookie)
{
  if (!obj) SETERRQ(1,"PetscObjectGetComm:Null object");
  *cookie = obj->cookie;
  return 0;
}

/*@
   PetscObjectExists - Determines whether a PETSc object has been destroyed.

   Input Parameter:
.  obj - any PETSc object, for example a Vec, Mat or KSP.

   Output Parameter:
.  exists - 0 if object does not exist; 1 if object does exist.

.keywords: object, exists
@*/
int PetscObjectExists(PetscObject obj,int *exists)
{
  *exists = 0;
  if (!obj) return 0;
  if (obj->cookie != PETSCFREEDHEADER) *exists = 1;
  return 0;
}

#if defined(__cplusplus)
extern "C" {
#endif
extern void sleep(int);
#if defined(__cplusplus)
}
#endif

/*@
   PetscSleep - Sleeps some number of seconds.

   Input Parameters:
.  s - number of seconds to sleep

.keywords: sleep, wait
@*/
void PetscSleep(int s)
{
  if (s < 0) getc(stdin);
  else       sleep(s);
}
/*
   PetscObjectSetPrefix - Sets the prefix used for searching for all 
   options of PetscObjectType in the database.

   Input Parameters:
.  obj - any PETSc object, for example a Vec, Mat or KSP.
.  prefix - the prefix string to prepend to option requests of the object.

.keywords: object, set, options, prefix, database
*/
int PetscObjectSetPrefix(PetscObject obj, char *prefix)
{
  if (!obj) SETERRQ(1,"PetscObjectSetOPrefix:Null object");
  if (obj->prefix) PetscFree(obj->prefix);
  obj->prefix = (char*) PetscMalloc((1+PetscStrlen(prefix))* \
                sizeof(char)); CHKPTRQ(obj->prefix);
  PetscStrcpy(obj->prefix,prefix);
  return 0;
}
/*
   PetscObjectAppendPrefix - Sets the prefix used for searching for all 
   options of PetscObjectType in the database.

   Input Parameters:
.  obj - any PETSc object, for example a Vec, Mat or KSP.
.  prefix - the prefix string to prepend to option requests of the object.

.keywords: object, append, options, prefix, database
*/
int PetscObjectAppendPrefix(PetscObject obj, char *prefix)
{
  char *buf = obj->prefix ;
  if (!obj) SETERRQ(1,"PetscObjectAppendOPrefix:Null object");
  if (!buf) return PetscObjectSetPrefix(obj, prefix);
  
  obj->prefix = (char*)PetscMalloc((1 + PetscStrlen(prefix) + PetscStrlen(buf))* \
                sizeof(char));  CHKPTRQ(obj->prefix);
  PetscStrcpy(obj->prefix,buf);
  PetscStrcat(obj->prefix,prefix);
  PetscFree(buf);
  return 0;
}
/*
   PetscObjectGetPrefix - Gets the prefix of the PetscObject.

   Input Parameters:
.  obj - any PETSc object, for example a Vec, Mat or KSP.

   Output Parameters:
.  prefix - pointer to the prefix string used is returned

.keywords: object, get, options, prefix, database
*/
int PetscObjectGetPrefix(PetscObject obj ,char** prefix)
{
  *prefix = obj->prefix;
  return 0;
}
