#ifndef lint
static char vcid[] = "$Id: flush.c,v 1.8 1997/02/22 02:28:36 bsmith Exp balay $";
#endif

#include "petsc.h"  /*I "petsc.h" I*/

struct _p_Viewer {
  PETSCHEADER
  int         (*flush)(Viewer);
};

#undef __FUNC__  
#define __FUNC__ "ViewerFlush" /* ADIC Ignore */
/*@
   ViewerFlush - Flushes a viewer (i.e. tries to dump all the 
   data that has been printed through a viewer).

   Input Parameters:
.  viewer - the viewer to be flushed

.keywords: Viewer, flush

.seealso: ViewerMatlabOpen(), ViewerFileOpenASCII()
@*/
int ViewerFlush(Viewer v)
{
  PetscValidHeaderSpecific(v,VIEWER_COOKIE);
  if (v->flush) return (*v->flush)(v);
  return 0;
}


