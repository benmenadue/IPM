
/** HEADER_BEGIN **/

#include <mpi.h>
#include <sys/stat.h>

#include "ipm.h"
#include "hashtable.h"
#include "ipm_core.h"
#include "mod_mpi.h"
#include "GEN.calltable_mpi.h"

#include "regstack.h"

#ifdef HAVE_CALLPATH
#include "mod_callpath.h"
#endif

#ifdef HAVE_KEYHIST
#include "mod_keyhist.h"
#endif

#ifndef MPI3CONST
#if MPI_VERSION >= 3 
#define MPI3CONST const
#else
#define MPI3CONST 
#endif 
#endif


/** HEADER_END **/


/* ---- wrapping __CFNAME__ ---- */
/*
 * strings in the form __IDENTIFIER__ are replaced 
 * by the wrapper script
 *
 * CFNAME __CFNAME__
 * PCFNAME __PCFNAME__
 * CRET __CRET__
 * CPARAMS __CPARAMS__
 * CARGS __CARGS__
 * CARGFMT __CARGFMT__
 * CRETFMT __CRETFMT__
 * CFID __CFID__
 * GET_BYTES __GET_BYTES__
 * GET_SSIZE __GET_SSIZE__
 * GET_RSIZE __GET_RSIZE__
 * GET_RANK __GET_RANK__
 * RETURN_VALUE __RETURN_VALUE__

 */

static void IPM___CFNAME__(__CPARAMS__, double tstart, double tstop)
{
  int bytes, irank;
  double t;
  int csite, idx, idx2, regid; 
  IPM_KEY_TYPE key;

  t=tstop-tstart;
  
  bytes=0; irank=0;
  
  __GET_BYTES__(bytes); 
  __GET_RANK__(irank);

#ifndef KEEP_BYTES_ACCURACY 
  KEEP_ONLY_HIGH_3BITS(bytes);
#endif /* KEEP_BYTES_ACCURACY */    

  if( irank==MPI_ANY_SOURCE ) {
    irank = IPM_RANK_ANY_SOURCE;
  }
  
#ifdef HAVE_CALLPATH
  csite=get_callsite_id();
#else
  csite=0;
#endif 
  
  regid=ipm_rstackptr->id;
  
  /* check for out-of-range */
  if(irank<KEY_MIN_RANK || irank>KEY_MAX_RANK) 
    irank=IPM_RANK_NULL;

  if(csite<KEY_MIN_CALLSITE || csite>KEY_MAX_CALLSITE) 
    irank=IPM_CALLSITE_NULL;

  if(regid<KEY_MIN_REGION || regid>KEY_MAX_REGION) 
    irank=IPM_REGION_NULL;

  if( bytes<0 ) {
    bytes=0;
    irank=IPM_RANK_NULL;
  }

#ifdef HAVE_MPI_TRACE
#ifndef HAVE_KEYHIST
  IPM_MPI_TRACE(tstart, tstop,__CFID___GLOBAL, "__CFNAME__", 
		irank, bytes, regid, csite);
#endif /* HAVE_KEYHIST */
#endif

  
  IPM_MPI_KEY(key, __CFID___GLOBAL, irank, bytes, 
	      regid, csite);

#ifdef IPM_COLLECTIVE_DETAILS 

#if IS_COLLECTIVE_CALL_ID(__CFID__)
  {
    int itype;
    MPITYPE_TO_IPMTYPE(stype, itype);
    KEY_SET_DATATYPE(key,itype);
  }
#endif /* collective call */
  
#if (__CFID__ == MPI_REDUCE_ID) || (__CFID__ == MPI_SCAN_ID) || \
  (__CFID__ == MPI_ALLREDUCE_ID)
  { 
    int iop;
    MPIOP_TO_IPMOP(op, iop);
    KEY_SET_OPERATION(key,iop);
  }
#endif

#endif /* IPM_COLLECTIVE_DETAILS */
  
  IPM_HASH_HKEY(ipm_htable, key, idx); 
  
#ifdef HAVE_KEYHIST
#ifdef HAVE_MPI_TRACE
  KEYHIST_TRACE(task.tracefile, key);
#endif

  IPM_XHASH_HKEY(ipm_xhtable,last_hkey,key,idx2);
  ipm_xhtable[idx2].t_tot+=(tstart-last_tstamp);

#ifdef KEYHIST_FULL_TIMING 
  ipm_xhtable[idx2].time[ipm_xhtable[idx2].count] =
    (tstart-last_tstamp);
#endif /* KEYHIST_FULL_TIMING */

  ipm_xhtable[idx2].count++;
  KEY_ASSIGN(last_hkey,key);
  last_tstamp=tstop;
#endif

  IPM_HASHTABLE_ADD(idx,t);

  if ((ipm_calltable[__CFID__].attr & DATA_TX) != 0 ||
      (ipm_calltable[__CFID__].attr & DATA_TXRX) != 0)
    ipm_comm_heatmap[irank] += bytes;
 
#ifdef HAVE_SNAP
 IPM_SNAP;
#endif
}

__CRET__ __CFNAME__(__CPARAMS__)
{
  __CRET__ rv;
  double tstart, tstop;

  IPM_TIMESTAMP(tstart);
  rv = __PCFNAME__(__CARGS__);
  IPM_TIMESTAMP(tstop);

  if( ipm_state!=STATE_ACTIVE ) {
    return rv;
  }

  if( ipm_in_fortran_pmpi==IPM_NOT_IN_FORTRAN_PMPI ) {
    IPM___CFNAME__(__CARGS__, tstart, tstop);
  }
 
  return rv;
}




