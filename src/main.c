#include "morphing.h"
#include "compil.date"
#define BUCKSIZ 64


Info info;

static void excfun(int sigid) {
    fprintf(stdout,"\n Unexpected error:");  fflush(stdout);
    switch(sigid) {
        case SIGABRT:
            fprintf(stdout,"  Abnormal stop\n");  break;
        case SIGBUS:
            fprintf(stdout,"  Code error...\n");  break;
        case SIGFPE:
            fprintf(stdout,"  Floating-point exception\n"); break;
        case SIGILL:
            fprintf(stdout,"  Illegal instruction\n"); break;
        case SIGSEGV:
            fprintf(stdout,"  Segmentation fault.\n");  break;
        case SIGTERM:
        case SIGINT:
            fprintf(stdout,"  Programm killed.\n");  break;
    }
    fprintf(stdout," No data file saved.\n");
    exit(1);
}

static void usage(char *prog) {
    fprintf(stdout,"\n usage: %s [-v[n]] [-h] [opts..] filein[.mesh]\n",prog);
    
    fprintf(stdout,"\n** Generic options :\n");
    fprintf(stdout,"-d       Turn on debug mode\n");
    fprintf(stdout,"-h       Print this message\n");
    fprintf(stdout,"-v [n]   Tune level of verbosity\n");
    fprintf(stdout,"-t n     FE type: 0.P1, 1.P2\n");
    
    fprintf(stdout,"\n**  File specifications\n");
    fprintf(stdout,"-in  file  input triangulation\n");
    fprintf(stdout,"-sol file  load initial solution\n");
    
    fprintf(stdout,"\n** Parameters\n");
    fprintf(stdout,"-err e   accuracy (default %E)\n",LS_RES);
    fprintf(stdout,"-nit n   iterations (default %d)\n",LS_MAXIT);
    fprintf(stdout,"-cpu n   CPUs\n");
    
    exit(1);
}

static int parsar(int argc, char *argv[], pMesh mesh_distance, pMesh mesh_omega0) {
    int i;
    
    i = 1;
    while ( i < argc ) {
        if ( *argv[i] == '-' ) {
            switch(argv[i][1]) {
                case 'h':  /* on-line help */
                case '?':
                    usage(argv[0]);
                    break;
                case 'c':
                    if ( !strcmp(argv[i],"-cpu") ) {
                        ++i;
                        if ( i == argc )
                            fprintf(stderr,"  ** missing argument\n");
                        else {
                            if ( isdigit(argv[i][0]) )
                                info.ncpu = atoi(argv[i]);
                            else
                                fprintf(stderr,"  ** argument [%s] discarded, use default: %d\n",argv[i],info.ncpu);
                        }
                    }
                    break;
                case 'd':  /* debug */
                    info.ddebug = 1;
                    break;
                case 'e':
                    if ( !strcmp(argv[i],"-err") ) {
                        ++i;
                        if ( isdigit(argv[i][0]) )
                            info.err = strtod(argv[i],NULL);
                        else
                            --i;
                    }
                    break;
                case 'n':
                    if ( !strcmp(argv[i],"-nit") ) {
                        ++i;
                        if ( i < argc && isdigit(argv[i][0]) )
                            info.nit = atoi(argv[i]);
                        else
                            --i;
                    }
                    break;
                case 'v':
                    if ( ++i < argc ) {
                        if ( argv[i][0] == '-' || isdigit(argv[i][0]) )
                            info.imprim = atoi(argv[i]);
                        else
                            i--;
                    }
                    else {
                        fprintf(stderr,"Missing argument option %c\n",argv[i-1][1]);
                        usage(argv[0]);
                    }
                    break;
                default:
                    fprintf(stderr,"  Unrecognized option %s\n",argv[i]);
                    usage(argv[0]);
            }
        }
        else {
            if ( mesh_distance->name == NULL ) {
                mesh_distance->name = argv[i];
                if ( info.imprim == -99 )  info.imprim = 5;
            }
            else if ( mesh_omega0->name == NULL ) {
                mesh_omega0->name = argv[i];
                if ( info.imprim == -99 )  info.imprim = 5;
            }
            else {
                fprintf(stdout,"  Argument %s ignored\n",argv[i]);
                usage(argv[0]);
            }
        }
        i++;
    }
    
    /* check params */
    if ( info.imprim == -99 ) {
        fprintf(stdout,"\n  -- PRINT (0 10(advised) -10) ?\n");
        fflush(stdin);
        fscanf(stdin,"%d",&i);
        info.imprim = i;
    }
    
    if ( mesh_distance->name == NULL ) {
        fprintf(stdout,"  -- MESH DISTANCE BASENAME ?\n");
        fflush(stdin);
        fscanf(stdin,"%s",mesh_distance->name);
    }
    
    if ( mesh_omega0->name == NULL ) {
        fprintf(stdout,"  -- MESH OMEGA 0 BASENAME ?\n");
        fflush(stdin);
        fscanf(stdin,"%s",mesh_omega0->name);
    }
  
 
  
    return 1;
}

//static int parsop(pMesh mesh, pSol sol) {
//    Cl         *pcl;
//    Mat        *pm;
//    float      fp1, fp2;
//    int        i, j, ncld,ret;
//    char       *ptr, buf[256], data[256];
//    FILE       *in;
//    
//    strcpy(data,mesh->name);
//    ptr = strstr(data,".mesh");
//    if ( ptr )  *ptr = '\0';
//    strcat(data,".elas");
//    in = fopen(data,"r");
//    if ( !in ) {
//        sprintf(data,"%s","DEFAULT.elas");
//        in = fopen(data,"r");
//        if ( !in )  return(1);
//    }
//    fprintf(stdout,"  %%%% %s OPENED\n",data);
//    
//    /* read parameters */
//    sol->nbcl = 0;
//    while ( !feof(in) ) {
//        ret = fscanf(in,"%s",data);
//        if ( !ret || feof(in) )  break;
//        for (i=0; i<strlen(data); i++) data[i] = tolower(data[i]);
//        
//        /* check for condition type */
//        if ( !strcmp(data,"dirichlet")  || !strcmp(data,"load") ) {
//            fscanf(in,"%d",&ncld);
//            for (i=sol->nbcl; i<sol->nbcl+ncld; i++) {
//                pcl = &sol->cl[i];
//                if ( !strcmp(data,"load") )             pcl->typ = Load;
//                else  if ( !strcmp(data,"dirichlet") )  pcl->typ = Dirichlet;
//                else {
//                    fprintf(stdout,"  %%%% Unknown condition: %s\n",data);
//                    continue;
//                }
//                
//                /* check for entity */
//                fscanf(in,"%d %s ",&pcl->ref,buf);
//                for (j=0; j<strlen(buf); j++)  buf[j] = tolower(buf[j]);
//                fscanf(in,"%c",&pcl->att);
//                pcl->att = tolower(pcl->att);
//                if ( (pcl->typ == Dirichlet) && (pcl->att != 'v' && pcl->att != 'f') ) {
//                    fprintf(stdout,"  %%%% Wrong format: %s\n",buf);
//                    continue;
//                }
//                else if ( (pcl->typ == Load) && (pcl->att != 'v' && pcl->att != 'f' && pcl->att != 'n') ) {
//                    fprintf(stdout,"  %%%% Wrong format: %s\n",buf);
//                    continue;
//                }
//                if ( !strcmp(buf,"vertices") || !strcmp(buf,"vertex") )          pcl->elt = LS_Ver;
//                else if ( !strcmp(buf,"edges") || !strcmp(buf,"edge") )          pcl->elt = LS_Edg;
//                else if ( !strcmp(buf,"triangles") || !strcmp(buf,"triangle") )  pcl->elt = LS_Tri;
//                
//                if ( pcl->att != 'f' && pcl->att != 'n' ) {
//                    for (j=0; j<mesh->dim; j++) {
//                        fscanf(in,"%f ",&fp1);
//                        pcl->u[j] = fp1;
//                    }
//                }
//                else if ( pcl->att == 'n' ) {
//                    fscanf(in,"%f ",&fp1);
//                    pcl->u[0] = fp1;
//                }
//            }
//            sol->nbcl += ncld;
//        }
//        /* gravity or body force */
//        else if ( !strcmp(data,"gravity") ) {
//            info.load |= (1 << 0);
//            for (j=0; j<mesh->dim; j++) {
//                fscanf(in,"%f ",&fp1);
//                info.gr[j] = fp1;
//            }
//        }
//        else if ( !strcmp(data,"lame") ) {
//            fscanf(in,"%d",&ncld);
//            assert(ncld <= LS_MAT);
//            sol->nmat = ncld;
//            for (i=0; i<ncld; i++) {
//                pm = &sol->mat[i];
//                fscanf(in,"%d %f %f\n",&pm->ref,&fp1,&fp2);
//                pm->lambda = fp1;
//                pm->mu     = fp2;
//            }
//        }
//        else if ( !strcmp(data,"youngpoisson") ) {
//            fscanf(in,"%d",&ncld);
//            sol->nmat = ncld;
//            for (i=0; i<ncld; i++) {
//                pm = &sol->mat[i];
//                fscanf(in,"%d %f %f\n",&pm->ref,&fp1,&fp2);
//                pm->lambda = (fp1 * fp2) / ((1.0+fp2) * (1.0-2.0*fp2));
//                pm->mu     = fp1 / (2.0*( 1.0+fp2));
//            }
//        }
//    }
//    fclose(in);
//    
//    for (i=0; i<sol->nbcl; i++) {
//        pcl = &sol->cl[i];
//        sol->cltyp |= pcl->elt;
//    }
//    return 1;
//}

static void endcod() {
    char stim[32];
    
    chrono(OFF,&info.ctim[0]);
    printim(info.ctim[0].gdif,stim);
    fprintf(stdout,"\n   ELAPSED TIME  %s\n",stim);
}

static void setfunc(int dim) {
    if ( dim == 2 ) {
        matA_P1   =  matA_P1_2d;
        hashelt   =  hashelt_2d;
        newBucket =  newBucket_2d;
        buckin    =  buckin_2d;
        locelt    =  locelt_2d;
        intpp1    =  intpp1_2d;
        evalderFunctional  = evalderFunctional2D;
        evalFunctional  = evalFunctional2D;
    }
    else {
        matA_P1   =  matA_P1_3d;
        hashelt   =  hashelt_3d;
        newBucket =  newBucket_3d;
        buckin    =  buckin_3d;
        locelt    =  locelt_3d;
        intpp1    =  intpp1_3d;
        evalderFunctional  = evalderFunctional3D;
        evalFunctional  = evalFunctional3D;
    }
}

FILE *out;

int main(int argc, char **argv) {
  
    Instance instance;
    int      ier;
    char     stim[32];
    pBucket  tmpBucket;
    fprintf(stdout,"  -- ELASTIC MORPHING, Release %s (%s) \n",LS_VER,LS_REL);
    fprintf(stdout,"     %s\n",LS_CPY);
    fprintf(stdout,"    %s\n",COMPIL);
  
    out = fopen("funct.data","a+");
    
    /* trap exceptions */
    signal(SIGABRT,excfun);
    signal(SIGFPE,excfun);
    signal(SIGILL,excfun);
    signal(SIGSEGV,excfun);
    signal(SIGTERM,excfun);
    signal(SIGINT,excfun);
    signal(SIGBUS,excfun);
    atexit(endcod);
    
    tminit(info.ctim,TIMEMAX);
    chrono(ON,&info.ctim[0]);
    
    /* default values */
  
    memset(&instance.mesh_omega0,0,sizeof(Mesh));
    memset(&instance.mesh_distance,0,sizeof(Mesh));
    memset(&instance.sol_distance,0,sizeof(Sol));
    memset(&instance.sol_omega0,0,sizeof(Sol));
    
    info.imprim = -99;
    info.ddebug = 0;
    info.ncpu   = 1;
    
    /* command line */
    if ( !parsar(argc, argv, &instance.mesh_distance, &instance.mesh_omega0) )  return 1;
    
    /* load data */
    if ( info.imprim )   fprintf(stdout,"\n  -- INPUT DATA\n");
    chrono(ON,&info.ctim[1]);
    if ( !loadMesh(&instance.mesh_distance) )  return 1;
    if ( !loadMesh(&instance.mesh_omega0) )  return 1;
    
    instance.mesh_omega0.refb = MESH_REF_B;
    instance.mesh_omega0.ref = MESH_REF;
  
  double c[3];
  c[0] = 0.5;
  c[1] = 0.5;
  c[2] = 0.5;
  
  if(!initMesh_3d(&instance.mesh_omega0,c,0.25)) return 0;
  if ( !saveMesh(&instance.mesh_omega0, &instance.mesh_distance,1) )  return 0;
  assert(1<0);
  
  
////  /* CALCUL ERREUR */
//  double err = 0;
////  
////  //err = hausdorff_3d(&instance.mesh_distance,&instance.mesh_omega0);
////  //fprintf(stdout,"  -- Husdorff  %lf \n",err);
//  err = 0;
//  err= errdist_3d(&instance.mesh_omega0, &instance.mesh_distance);
//  fprintf(stdout,"  -- l2  %lf \n",err);
//  assert(1 < 0);
  
  
  
    instance.sol_distance.name = instance.mesh_distance.name;
    ier = loadSol(&instance.sol_distance);
    if ( !ier  )  return 1;
    if ( instance.sol_distance.np != instance.mesh_distance.np ) {
        fprintf(stdout,"  ## WARNING: WRONG SOLUTION NUMBER. IGNORED\n");
        return 0;
    }

   fprintf(stdout,"  -- sol size = %d \n",instance.sol_distance.size[0]);
    setfunc(instance.mesh_distance.dim);
    if (instance.mesh_distance.ne) {
        instance.mesh_distance.adja = (int*)calloc(4*instance.mesh_distance.ne+5,sizeof(int));
        assert(instance.mesh_distance.adja);
    }
    else if (instance.mesh_distance.nt) {
    	instance.mesh_distance.adja = (int*)calloc(3*instance.mesh_distance.nt+5,sizeof(int));
        assert(instance.mesh_distance.adja);
    }
    instance.sol_omega0.name = instance.mesh_omega0.name;
    instance.sol_omega0.np  = instance.mesh_omega0.np;
    instance.sol_omega0.mat = (Mat*)calloc(LS_MAT,sizeof(Mat));
    instance.sol_omega0.nit = LS_MAXIT;
    instance.sol_omega0.dim = instance.mesh_omega0.dim;
    instance.sol_omega0.ver = instance.mesh_omega0.ver;
    instance.sol_omega0.cl  = (Cl*)calloc(LS_CL,sizeof(Cl));
    instance.sol_omega0.err = LS_RES;
    instance.sol_omega0.u   = (double*)calloc(instance.sol_omega0.dim*(instance.sol_omega0.np),sizeof(double));
    assert(instance.sol_omega0.u);
    instance.sol_omega0.p   = (double*)calloc(instance.sol_omega0.dim*(instance.sol_omega0.np),sizeof(double));
    assert(instance.sol_omega0.p);
    instance.sol_omega0.d   = (double*)calloc(instance.sol_omega0.np,sizeof(double));
    assert(instance.sol_omega0.d);
    	
    //if ( !parsop(&instance.mesh_omega0,&instance.sol_omega0) )  return 1;
    
    chrono(OFF,&info.ctim[1]);
    printim(info.ctim[1].gdif,stim);
    fprintf(stdout,"  -- DATA READING COMPLETED.     %s \n",stim);
    
    chrono(ON,&info.ctim[2]);
    fprintf(stdout,"\n  %s\n   MODULE MORPHING : %s (%s)\n  %s\n",LS_STR,LS_VER,LS_REL,LS_STR);
    if ( info.imprim )   fprintf(stdout,"  -- PHASE 1 : INITIALISATION\n");

    if ( !hashelt(&instance.mesh_distance) )  return 1;
    tmpBucket = newBucket(&instance.mesh_distance,BUCKSIZ);
    instance.bucket = *tmpBucket;

    chrono(OFF,&info.ctim[2]);
    printim(info.ctim[2].gdif,stim);
    if ( info.imprim ) fprintf(stdout,"  -- PHASE 1 COMPLETED.     %s\n\n",stim);
  
    /* minimization */
    chrono(ON,&info.ctim[3]);
    if ( info.imprim ) fprintf(stdout,"  -- PHASE 2 : MINIMIZATION\n"); 
    if ( !minimization(&instance) )  return 1;
 
    chrono(OFF,&info.ctim[3]);
    printim(info.ctim[3].gdif,stim);
    if ( info.imprim ) fprintf(stdout,"  -- PHASE 2 COMPLETED.     %s\n",stim);
    
    /* free mem */
    free(instance.mesh_omega0.tetra);
    free(instance.mesh_omega0.point);
    free(instance.mesh_omega0.tria);
    free(instance.mesh_omega0.edge);
    free(instance.mesh_distance.tetra);
    free(instance.mesh_distance.point);
    free(instance.mesh_distance.tria);
    free(instance.mesh_distance.adja);
    free(instance.mesh_distance.edge);
    free(instance.sol_distance.u);
    free(instance.sol_distance.valp1);
    free(instance.sol_omega0.u);
    free(instance.sol_omega0.d);
    free(instance.sol_omega0.mat);
    free(instance.sol_omega0.cl);
    free(instance.sol_omega0.p);
    free(instance.bucket.head);
    free(instance.bucket.link);
    free(tmpBucket);

    chrono(OFF,&info.ctim[0]);
    printim(info.ctim[0].gdif,stim);
    fprintf(stdout,"\n  %s\n   END OF MODULE MORPHING. %s \n  %s\n",LS_STR,stim,LS_STR);
    
  
    return 0;
}

