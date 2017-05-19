#include "morphing.h"

#define BUCKSIZ 64

extern FILE *out;
extern Info info;


/* find best step */
int findstep(pInstance instance, double *step){
  double  J,J_temp;
    
  J = evalFunctional(instance,0);  
  J_temp = evalFunctional3D(instance,step[0]);
  
  while (J_temp>=J) {
    step[0] *= 0.8;
    J_temp = evalFunctional(instance,step[0]);
    if (step[0] < 1e-8) {
      return 0;
      }
    }
  return 1;

}

int minimization(pInstance instance){
  
    int    k,maxit;
    double *F,step;
  
    maxit = 1000;
  	step = 1e8;
  
    F = (double*)calloc(instance->mesh_omega0.dim*instance->mesh_omega0.np, sizeof(double));
    memset(instance->sol_omega0.u, 0, instance->mesh_omega0.dim*instance->sol_omega0.np*sizeof(double));
    assert(F);
  
  /* compute initial distance field */
  if ( !computedistance(instance) )  return 0;
  if ( !saveDistance(&instance->sol_omega0, &instance->mesh_omega0, &instance->mesh_distance,0) )  return 0;

  
  /*gradient descent iterations */
  for (k=1; k<=maxit; k++) {
    /*compute descent direction */
    	if ( !evalderFunctional(instance,F) )  return 0;
		  /* find best step */
    if ( !findstep(instance,&step) ){
			if ( !saveMesh(&instance->mesh_omega0, &instance->mesh_distance,1) )  return 0;
	      	if ( !computedistance(instance) )  return 0;
	      	if ( !saveDistance(&instance->sol_omega0, &instance->mesh_omega0, &instance->mesh_distance,1) )  return 0;
	      	if ( !saveSol(&instance->sol_omega0, &instance->mesh_omega0, &instance->mesh_distance, 1) )  return 0;
          return 0;
      }
    
    /*advect mesh */
	    if ( !moveMesh(&instance->mesh_omega0,&instance->sol_omega0,step) )  return 0;
	    
    if(k%20==0) step = 1e7;
  }
  
  /*free memory */
  free(F);
  
    return 1;
}
