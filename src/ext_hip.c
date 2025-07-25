/**
 * Author......: See docs/credits.txt
 * License.....: MIT
 */

#include "common.h"
#include "types.h"
#include "memory.h"
#include "event.h"
#include "ext_hip.h"
#include "shared.h"

#include "dynloader.h"

char *hipDllPath (char *hipSDKPath)
{
  /*
    AMD HIP DLLs is stored at "C:\Program Files\ROCm\X.Y\bin\amdhip64_X.dll"

    This function can return complete dll path based on major release version
    X.Y parsed from the ENV variable HIP_PATH.
  */

  const char *marker = "\\ROCm\\";

  int major = 0;
  int minor = 0;

  const char *version_start = strstr (hipSDKPath, marker);

  if (version_start == NULL) return NULL;

  version_start += strlen (marker); // now points at "6.2\\"

  if (sscanf (version_start, "%d.%d", &major, &minor) != 2) return NULL;

  char *hipdllpath = NULL;

  hc_asprintf (&hipdllpath, "%sbin\\amdhip64_%d.dll", hipSDKPath, major);

  return (hipdllpath);
}

int hip_init (void *hashcat_ctx)
{
  backend_ctx_t *backend_ctx = ((hashcat_ctx_t *) hashcat_ctx)->backend_ctx;

  HIP_PTR *hip = (HIP_PTR *) backend_ctx->hip;

  memset (hip, 0, sizeof (HIP_PTR));

  #if   defined (_WIN)
  char *hipSDKPath = getenv ("HIP_PATH");

  if (hipSDKPath == NULL) return -1;

  char *hipdllpath = hipDllPath (hipSDKPath);

  if (hipdllpath == NULL) return -1;

  hip->lib = hc_dlopen (hipdllpath);

  free (hipdllpath);

  if (hip->lib == NULL)
  {
    hip->lib = hc_dlopen ("amdhip64.dll");
  }

  #elif defined (__APPLE__)
  hip->lib = hc_dlopen ("fixme.dylib");
  #elif defined (__CYGWIN__)
  char *hipSDKPath = getenv ("HIP_PATH");

  if (hipSDKPath == NULL) return -1;

  char *hipdllpath = hipDllPath (hipSDKPath);

  if (hipdllpath == NULL) return -1;

  hip->lib = hc_dlopen (hipdllpath);

  free (hipdllpath);

  if (hip->lib == NULL)
  {
    hip->lib = hc_dlopen ("amdhip64.dll");
  }

  #else
  hip->lib = hc_dlopen ("libamdhip64.so");
  #endif

  if (hip->lib == NULL) return -1;

  // finding the right symbol is a PITA,
  #define HC_LOAD_FUNC_HIP(ptr,name,hipname,type,libname,noerr) \
    do { \
      ptr->name = (type) hc_dlsym ((ptr)->lib, #hipname); \
      if ((noerr) != -1) { \
        if (!(ptr)->name) { \
          if ((noerr) == 1) { \
            event_log_error (hashcat_ctx, "%s is missing from %s shared library.", #name, #libname); \
            return -1; \
          } \
          if ((noerr) != 1) { \
            event_log_warning (hashcat_ctx, "%s is missing from %s shared library.", #name, #libname); \
            return 0; \
          } \
        } \
      } \
    } while (0)

  // finding the right symbol is a PITA, because of the _v2 suffix
  // a good reference is cuda.h itself
  // this needs to be verified for each new cuda release

  HC_LOAD_FUNC_HIP (hip, hipCtxCreate,              hipCtxCreate,                 HIP_HIPCTXCREATE,               HIP, 1);
  HC_LOAD_FUNC_HIP (hip, hipCtxDestroy,             hipCtxDestroy,                HIP_HIPCTXDESTROY,              HIP, 1);
  HC_LOAD_FUNC_HIP (hip, hipCtxPopCurrent,          hipCtxPopCurrent,             HIP_HIPCTXPOPCURRENT,           HIP, 1);
  HC_LOAD_FUNC_HIP (hip, hipCtxPushCurrent,         hipCtxPushCurrent,            HIP_HIPCTXPUSHCURRENT,          HIP, 1);
  HC_LOAD_FUNC_HIP (hip, hipCtxSetCurrent,          hipCtxSetCurrent,             HIP_HIPCTXSETCURRENT,           HIP, 1);
  HC_LOAD_FUNC_HIP (hip, hipCtxSynchronize,         hipCtxSynchronize,            HIP_HIPCTXSYNCHRONIZE,          HIP, 1);
  HC_LOAD_FUNC_HIP (hip, hipDeviceGet,              hipDeviceGet,                 HIP_HIPDEVICEGET,               HIP, 1);
  HC_LOAD_FUNC_HIP (hip, hipDeviceGetAttribute,     hipDeviceGetAttribute,        HIP_HIPDEVICEGETATTRIBUTE,      HIP, 1);
  HC_LOAD_FUNC_HIP (hip, hipDeviceGetCount,         hipGetDeviceCount,            HIP_HIPDEVICEGETCOUNT,          HIP, 1);
  HC_LOAD_FUNC_HIP (hip, hipDeviceGetName,          hipDeviceGetName,             HIP_HIPDEVICEGETNAME,           HIP, 1);
  HC_LOAD_FUNC_HIP (hip, hipDeviceTotalMem,         hipDeviceTotalMem,            HIP_HIPDEVICETOTALMEM,          HIP, 1);
  HC_LOAD_FUNC_HIP (hip, hipDriverGetVersion,       hipDriverGetVersion,          HIP_HIPDRIVERGETVERSION,        HIP, 1);
  HC_LOAD_FUNC_HIP (hip, hipEventCreate,            hipEventCreate,               HIP_HIPEVENTCREATE,             HIP, 1);
  HC_LOAD_FUNC_HIP (hip, hipEventCreateWithFlags,   hipEventCreateWithFlags,      HIP_HIPEVENTCREATEWITHFLAGS,    HIP, 1);
  HC_LOAD_FUNC_HIP (hip, hipEventDestroy,           hipEventDestroy,              HIP_HIPEVENTDESTROY,            HIP, 1);
  HC_LOAD_FUNC_HIP (hip, hipEventElapsedTime,       hipEventElapsedTime,          HIP_HIPEVENTELAPSEDTIME,        HIP, 1);
  HC_LOAD_FUNC_HIP (hip, hipEventRecord,            hipEventRecord,               HIP_HIPEVENTRECORD,             HIP, 1);
  HC_LOAD_FUNC_HIP (hip, hipEventSynchronize,       hipEventSynchronize,          HIP_HIPEVENTSYNCHRONIZE,        HIP, 1);
  HC_LOAD_FUNC_HIP (hip, hipFuncGetAttribute,       hipFuncGetAttribute,          HIP_HIPFUNCGETATTRIBUTE,        HIP, 1);
  HC_LOAD_FUNC_HIP (hip, hipGetErrorName,           hipDrvGetErrorName,           HIP_HIPGETERRORNAME,            HIP, 1);
  HC_LOAD_FUNC_HIP (hip, hipGetErrorString,         hipDrvGetErrorString,         HIP_HIPGETERRORSTRING,          HIP, 1);
  HC_LOAD_FUNC_HIP (hip, hipInit,                   hipInit,                      HIP_HIPINIT,                    HIP, 1);
  HC_LOAD_FUNC_HIP (hip, hipLaunchKernel,           hipModuleLaunchKernel,        HIP_HIPLAUNCHKERNEL,            HIP, 1);
  HC_LOAD_FUNC_HIP (hip, hipMemAlloc,               hipMalloc,                    HIP_HIPMEMALLOC,                HIP, 1);
  HC_LOAD_FUNC_HIP (hip, hipMemFree,                hipFree,                      HIP_HIPMEMFREE,                 HIP, 1);
  HC_LOAD_FUNC_HIP (hip, hipMemGetInfo,             hipMemGetInfo,                HIP_HIPMEMGETINFO,              HIP, 1);
  HC_LOAD_FUNC_HIP (hip, hipMemcpyDtoD,             hipMemcpyDtoD,                HIP_HIPMEMCPYDTOD,              HIP, 1);
  HC_LOAD_FUNC_HIP (hip, hipMemcpyDtoH,             hipMemcpyDtoH,                HIP_HIPMEMCPYDTOH,              HIP, 1);
  HC_LOAD_FUNC_HIP (hip, hipMemcpyHtoD,             hipMemcpyHtoD,                HIP_HIPMEMCPYHTOD,              HIP, 1);
  HC_LOAD_FUNC_HIP (hip, hipMemsetD32,              hipMemsetD32,                 HIP_HIPMEMSETD32,               HIP, 1);
  HC_LOAD_FUNC_HIP (hip, hipMemsetD8,               hipMemsetD8,                  HIP_HIPMEMSETD8,                HIP, 1);
  HC_LOAD_FUNC_HIP (hip, hipMemcpyDtoDAsync,        hipMemcpyDtoDAsync,           HIP_HIPMEMCPYDTODASYNC,         HIP, 1);
  HC_LOAD_FUNC_HIP (hip, hipMemcpyDtoHAsync,        hipMemcpyDtoHAsync,           HIP_HIPMEMCPYDTOHASYNC,         HIP, 1);
  HC_LOAD_FUNC_HIP (hip, hipMemcpyHtoDAsync,        hipMemcpyHtoDAsync,           HIP_HIPMEMCPYHTODASYNC,         HIP, 1);
  HC_LOAD_FUNC_HIP (hip, hipMemsetD32Async,         hipMemsetD32Async,            HIP_HIPMEMSETD32ASYNC,          HIP, 1);
  HC_LOAD_FUNC_HIP (hip, hipMemsetD8Async,          hipMemsetD8Async,             HIP_HIPMEMSETD8ASYNC,           HIP, 1);
  HC_LOAD_FUNC_HIP (hip, hipModuleGetFunction,      hipModuleGetFunction,         HIP_HIPMODULEGETFUNCTION,       HIP, 1);
  HC_LOAD_FUNC_HIP (hip, hipModuleGetGlobal,        hipModuleGetGlobal,           HIP_HIPMODULEGETGLOBAL,         HIP, 1);
  HC_LOAD_FUNC_HIP (hip, hipModuleLoadDataEx,       hipModuleLoadDataEx,          HIP_HIPMODULELOADDATAEX,        HIP, 1);
  HC_LOAD_FUNC_HIP (hip, hipModuleUnload,           hipModuleUnload,              HIP_HIPMODULEUNLOAD,            HIP, 1);
  HC_LOAD_FUNC_HIP (hip, hipRuntimeGetVersion,      hipRuntimeGetVersion,         HIP_HIPRUNTIMEGETVERSION,       HIP, 1);
  HC_LOAD_FUNC_HIP (hip, hipSetDevice,              hipSetDevice,                 HIP_HIPSETDEVICE,               HIP, 1);
  HC_LOAD_FUNC_HIP (hip, hipSetDeviceFlags,         hipSetDeviceFlags,            HIP_HIPSETDEVICEFLAGS,          HIP, 1);
  HC_LOAD_FUNC_HIP (hip, hipStreamCreate,           hipStreamCreate,              HIP_HIPSTREAMCREATE,            HIP, 1);
  HC_LOAD_FUNC_HIP (hip, hipStreamCreateWithFlags,  hipStreamCreateWithFlags,     HIP_HIPSTREAMCREATEWITHFLAGS,   HIP, 1);
  HC_LOAD_FUNC_HIP (hip, hipStreamDestroy,          hipStreamDestroy,             HIP_HIPSTREAMDESTROY,           HIP, 1);
  HC_LOAD_FUNC_HIP (hip, hipStreamSynchronize,      hipStreamSynchronize,         HIP_HIPSTREAMSYNCHRONIZE,       HIP, 1);
  HC_LOAD_FUNC_HIP (hip, hipGetDeviceProperties,    hipGetDevicePropertiesR0600,  HIP_HIPGETDEVICEPROPERTIES,     HIP, 1);
  HC_LOAD_FUNC_HIP (hip, hipModuleOccupancyMaxActiveBlocksPerMultiprocessor, hipModuleOccupancyMaxActiveBlocksPerMultiprocessor, HIP_HIPMODULEOCCUPANCYMAXACTIVEBLOCKSPERMULTIPROCESSOR, HIP, 1);

  return 0;
}

void hip_close (void *hashcat_ctx)
{
  backend_ctx_t *backend_ctx = ((hashcat_ctx_t *) hashcat_ctx)->backend_ctx;

  HIP_PTR *hip = (HIP_PTR *) backend_ctx->hip;

  if (hip)
  {
    if (hip->lib)
    {
      hc_dlclose (hip->lib);
    }

    hcfree (backend_ctx->hip);

    backend_ctx->hip = NULL;
  }
}

int hc_hipEventDestroyPtr (void *hashcat_ctx, hipEvent_t *hEvent)
{
  int rc = -1;

  if (hEvent == NULL || *hEvent == NULL) return rc;

  rc = hc_hipEventDestroy (hashcat_ctx, *hEvent);

  *hEvent = NULL;

  return rc;
}

int hc_hipMemFreePtr (void *hashcat_ctx, hipDeviceptr_t *dptr)
{
  int rc = -1;

  if (dptr == NULL || *dptr == NULL) return rc;

  rc = hc_hipMemFree (hashcat_ctx, *dptr);

  *dptr = 0;

  return rc;
}

int hc_hipModuleUnloadPtr (void *hashcat_ctx, hipModule_t *hmod)
{
  int rc = -1;

  if (hmod == NULL || *hmod == NULL) return rc;

  rc = hc_hipModuleUnload (hashcat_ctx, *hmod);

  *hmod = NULL;

  return rc;
}

int hc_hipStreamDestroyPtr (void *hashcat_ctx, hipStream_t *hStream)
{
  int rc = -1;

  if (hStream == NULL || *hStream == NULL) return rc;

  rc = hc_hipStreamDestroy (hashcat_ctx, *hStream);

  *hStream = NULL;

  return rc;
}

int hc_hipCtxCreate (void *hashcat_ctx, hipCtx_t *pctx, unsigned int flags, hipDevice_t dev)
{
  backend_ctx_t *backend_ctx = ((hashcat_ctx_t *) hashcat_ctx)->backend_ctx;

  HIP_PTR *hip = (HIP_PTR *) backend_ctx->hip;

  const hipError_t HIP_err = hip->hipCtxCreate (pctx, flags, dev);

  if (HIP_err != hipSuccess)
  {
    const char *pStr = NULL;

    if (hip->hipGetErrorString (HIP_err, &pStr) == hipSuccess)
    {
      event_log_error (hashcat_ctx, "hipCtxCreate(): %s", pStr);
    }
    else
    {
      event_log_error (hashcat_ctx, "hipCtxCreate(): %d", HIP_err);
    }

    return -1;
  }

  return 0;
}

int hc_hipCtxDestroy (void *hashcat_ctx, hipCtx_t ctx)
{
  backend_ctx_t *backend_ctx = ((hashcat_ctx_t *) hashcat_ctx)->backend_ctx;

  HIP_PTR *hip = (HIP_PTR *) backend_ctx->hip;

  const hipError_t HIP_err = hip->hipCtxDestroy (ctx);

  if (HIP_err != hipSuccess)
  {
    const char *pStr = NULL;

    if (hip->hipGetErrorString (HIP_err, &pStr) == hipSuccess)
    {
      event_log_error (hashcat_ctx, "hipCtxDestroy(): %s", pStr);
    }
    else
    {
      event_log_error (hashcat_ctx, "hipCtxDestroy(): %d", HIP_err);
    }

    return -1;
  }

  return 0;
}

int hc_hipCtxPopCurrent (void *hashcat_ctx, hipCtx_t *pctx)
{
  backend_ctx_t *backend_ctx = ((hashcat_ctx_t *) hashcat_ctx)->backend_ctx;

  HIP_PTR *hip = (HIP_PTR *) backend_ctx->hip;

  const hipError_t HIP_err = hip->hipCtxPopCurrent (pctx);

  if (HIP_err != hipSuccess)
  {
    const char *pStr = NULL;

    if (hip->hipGetErrorString (HIP_err, &pStr) == hipSuccess)
    {
      event_log_error (hashcat_ctx, "hipCtxPopCurrent(): %s", pStr);
    }
    else
    {
      event_log_error (hashcat_ctx, "hipCtxPopCurrent(): %d", HIP_err);
    }

    return -1;
  }

  return 0;
}

int hc_hipCtxPushCurrent (void *hashcat_ctx, hipCtx_t ctx)
{
  backend_ctx_t *backend_ctx = ((hashcat_ctx_t *) hashcat_ctx)->backend_ctx;

  HIP_PTR *hip = (HIP_PTR *) backend_ctx->hip;

  const hipError_t HIP_err = hip->hipCtxPushCurrent (ctx);

  if (HIP_err != hipSuccess)
  {
    const char *pStr = NULL;

    if (hip->hipGetErrorString (HIP_err, &pStr) == hipSuccess)
    {
      event_log_error (hashcat_ctx, "hipCtxPushCurrent(): %s", pStr);
    }
    else
    {
      event_log_error (hashcat_ctx, "hipCtxPushCurrent(): %d", HIP_err);
    }

    return -1;
  }

  return 0;
}

int hc_hipCtxSetCurrent (void *hashcat_ctx, hipCtx_t ctx)
{
  backend_ctx_t *backend_ctx = ((hashcat_ctx_t *) hashcat_ctx)->backend_ctx;

  HIP_PTR *hip = (HIP_PTR *) backend_ctx->hip;

  const hipError_t HIP_err = hip->hipCtxSetCurrent (ctx);

  if (HIP_err != hipSuccess)
  {
    const char *pStr = NULL;

    if (hip->hipGetErrorString (HIP_err, &pStr) == hipSuccess)
    {
      event_log_error (hashcat_ctx, "hipCtxSetCurrent(): %s", pStr);
    }
    else
    {
      event_log_error (hashcat_ctx, "hipCtxSetCurrent(): %d", HIP_err);
    }

    return -1;
  }

  return 0;
}

int hc_hipCtxSynchronize (void *hashcat_ctx)
{
  backend_ctx_t *backend_ctx = ((hashcat_ctx_t *) hashcat_ctx)->backend_ctx;

  HIP_PTR *hip = (HIP_PTR *) backend_ctx->hip;

  const hipError_t HIP_err = hip->hipCtxSynchronize ();

  if (HIP_err != hipSuccess)
  {
    const char *pStr = NULL;

    if (hip->hipGetErrorString (HIP_err, &pStr) == hipSuccess)
    {
      event_log_error (hashcat_ctx, "hipCtxSynchronize(): %s", pStr);
    }
    else
    {
      event_log_error (hashcat_ctx, "hipCtxSynchronize(): %d", HIP_err);
    }

    return -1;
  }

  return 0;
}

int hc_hipDeviceGet (void *hashcat_ctx, hipDevice_t* device, int ordinal)
{
  backend_ctx_t *backend_ctx = ((hashcat_ctx_t *) hashcat_ctx)->backend_ctx;

  HIP_PTR *hip = (HIP_PTR *) backend_ctx->hip;

  const hipError_t HIP_err = hip->hipDeviceGet (device, ordinal);

  if (HIP_err != hipSuccess)
  {
    const char *pStr = NULL;

    if (hip->hipGetErrorString (HIP_err, &pStr) == hipSuccess)
    {
      event_log_error (hashcat_ctx, "hipDeviceGet(): %s", pStr);
    }
    else
    {
      event_log_error (hashcat_ctx, "hipDeviceGet(): %d", HIP_err);
    }

    return -1;
  }

  return 0;
}

int hc_hipDeviceGetAttribute (void *hashcat_ctx, int *pi, hipDeviceAttribute_t attrib, hipDevice_t dev)
{
  backend_ctx_t *backend_ctx = ((hashcat_ctx_t *) hashcat_ctx)->backend_ctx;

  HIP_PTR *hip = (HIP_PTR *) backend_ctx->hip;

  const hipError_t HIP_err = hip->hipDeviceGetAttribute (pi, attrib, dev);

  if (HIP_err != hipSuccess)
  {
    const char *pStr = NULL;

    if (hip->hipGetErrorString (HIP_err, &pStr) == hipSuccess)
    {
      event_log_error (hashcat_ctx, "hipDeviceGetAttribute(): %s", pStr);
    }
    else
    {
      event_log_error (hashcat_ctx, "hipDeviceGetAttribute(): %d", HIP_err);
    }

    return -1;
  }

  return 0;
}

int hc_hipDeviceGetCount (void *hashcat_ctx, int *count)
{
  backend_ctx_t *backend_ctx = ((hashcat_ctx_t *) hashcat_ctx)->backend_ctx;

  HIP_PTR *hip = (HIP_PTR *) backend_ctx->hip;

  const hipError_t HIP_err = hip->hipDeviceGetCount (count);

  if (HIP_err != hipSuccess)
  {
    const char *pStr = NULL;

    if (hip->hipGetErrorString (HIP_err, &pStr) == hipSuccess)
    {
      event_log_error (hashcat_ctx, "hipDeviceGetCount(): %s", pStr);
    }
    else
    {
      event_log_error (hashcat_ctx, "hipDeviceGetCount(): %d", HIP_err);
    }

    return -1;
  }

  return 0;
}

int hc_hipDeviceGetName (void *hashcat_ctx, char *name, int len, hipDevice_t dev)
{
  backend_ctx_t *backend_ctx = ((hashcat_ctx_t *) hashcat_ctx)->backend_ctx;

  HIP_PTR *hip = (HIP_PTR *) backend_ctx->hip;

  const hipError_t HIP_err = hip->hipDeviceGetName (name, len, dev);

  if (HIP_err != hipSuccess)
  {
    const char *pStr = NULL;

    if (hip->hipGetErrorString (HIP_err, &pStr) == hipSuccess)
    {
      event_log_error (hashcat_ctx, "hipDeviceGetName(): %s", pStr);
    }
    else
    {
      event_log_error (hashcat_ctx, "hipDeviceGetName(): %d", HIP_err);
    }

    return -1;
  }

  return 0;
}

int hc_hipDeviceTotalMem (void *hashcat_ctx, size_t *bytes, hipDevice_t dev)
{
  backend_ctx_t *backend_ctx = ((hashcat_ctx_t *) hashcat_ctx)->backend_ctx;

  HIP_PTR *hip = (HIP_PTR *) backend_ctx->hip;

  const hipError_t HIP_err = hip->hipDeviceTotalMem (bytes, dev);

  if (HIP_err != hipSuccess)
  {
    const char *pStr = NULL;

    if (hip->hipGetErrorString (HIP_err, &pStr) == hipSuccess)
    {
      event_log_error (hashcat_ctx, "hipDeviceTotalMem(): %s", pStr);
    }
    else
    {
      event_log_error (hashcat_ctx, "hipDeviceTotalMem(): %d", HIP_err);
    }

    return -1;
  }

  return 0;
}

int hc_hipDriverGetVersion (void *hashcat_ctx, int *driverVersion)
{
  backend_ctx_t *backend_ctx = ((hashcat_ctx_t *) hashcat_ctx)->backend_ctx;

  HIP_PTR *hip = (HIP_PTR *) backend_ctx->hip;

  const hipError_t HIP_err = hip->hipDriverGetVersion (driverVersion);

  if (HIP_err != hipSuccess)
  {
    const char *pStr = NULL;

    if (hip->hipGetErrorString (HIP_err, &pStr) == hipSuccess)
    {
      event_log_error (hashcat_ctx, "hipDriverGetVersion(): %s", pStr);
    }
    else
    {
      event_log_error (hashcat_ctx, "hipDriverGetVersion(): %d", HIP_err);
    }

    return -1;
  }

  return 0;
}

int hc_hipEventCreate (void *hashcat_ctx, hipEvent_t *phEvent)
{
  backend_ctx_t *backend_ctx = ((hashcat_ctx_t *) hashcat_ctx)->backend_ctx;

  HIP_PTR *hip = (HIP_PTR *) backend_ctx->hip;

  const hipError_t HIP_err = hip->hipEventCreate (phEvent);

  if (HIP_err != hipSuccess)
  {
    const char *pStr = NULL;

    if (hip->hipGetErrorString (HIP_err, &pStr) == hipSuccess)
    {
      event_log_error (hashcat_ctx, "hipEventCreate(): %s", pStr);
    }
    else
    {
      event_log_error (hashcat_ctx, "hipEventCreate(): %d", HIP_err);
    }

    return -1;
  }

  return 0;
}

int hc_hipEventCreateWithFlags (void *hashcat_ctx, hipEvent_t *phEvent, unsigned int flags)
{
  backend_ctx_t *backend_ctx = ((hashcat_ctx_t *) hashcat_ctx)->backend_ctx;

  HIP_PTR *hip = (HIP_PTR *) backend_ctx->hip;

  const hipError_t HIP_err = hip->hipEventCreateWithFlags (phEvent, flags);

  if (HIP_err != hipSuccess)
  {
    const char *pStr = NULL;

    if (hip->hipGetErrorString (HIP_err, &pStr) == hipSuccess)
    {
      event_log_error (hashcat_ctx, "hipEventCreateWithFlags(): %s", pStr);
    }
    else
    {
      event_log_error (hashcat_ctx, "hipEventCreateWithFlags(): %d", HIP_err);
    }

    return -1;
  }

  return 0;
}

int hc_hipEventDestroy (void *hashcat_ctx, hipEvent_t hEvent)
{
  backend_ctx_t *backend_ctx = ((hashcat_ctx_t *) hashcat_ctx)->backend_ctx;

  HIP_PTR *hip = (HIP_PTR *) backend_ctx->hip;

  const hipError_t HIP_err = hip->hipEventDestroy (hEvent);

  if (HIP_err != hipSuccess)
  {
    const char *pStr = NULL;

    if (hip->hipGetErrorString (HIP_err, &pStr) == hipSuccess)
    {
      event_log_error (hashcat_ctx, "hipEventDestroy(): %s", pStr);
    }
    else
    {
      event_log_error (hashcat_ctx, "hipEventDestroy(): %d", HIP_err);
    }

    return -1;
  }

  return 0;
}

int hc_hipEventElapsedTime (void *hashcat_ctx, float *pMilliseconds, hipEvent_t hStart, hipEvent_t hEnd)
{
  backend_ctx_t *backend_ctx = ((hashcat_ctx_t *) hashcat_ctx)->backend_ctx;

  HIP_PTR *hip = (HIP_PTR *) backend_ctx->hip;

  const hipError_t HIP_err = hip->hipEventElapsedTime (pMilliseconds, hStart, hEnd);

  if (HIP_err != hipSuccess)
  {
    const char *pStr = NULL;

    if (hip->hipGetErrorString (HIP_err, &pStr) == hipSuccess)
    {
      event_log_error (hashcat_ctx, "hipEventElapsedTime(): %s", pStr);
    }
    else
    {
      event_log_error (hashcat_ctx, "hipEventElapsedTime(): %d", HIP_err);
    }

    return -1;
  }

  return 0;
}

int hc_hipEventRecord (void *hashcat_ctx, hipEvent_t hEvent, hipStream_t hStream)
{
  backend_ctx_t *backend_ctx = ((hashcat_ctx_t *) hashcat_ctx)->backend_ctx;

  HIP_PTR *hip = (HIP_PTR *) backend_ctx->hip;

  const hipError_t HIP_err = hip->hipEventRecord (hEvent, hStream);

  if (HIP_err != hipSuccess)
  {
    const char *pStr = NULL;

    if (hip->hipGetErrorString (HIP_err, &pStr) == hipSuccess)
    {
      event_log_error (hashcat_ctx, "hipEventRecord(): %s", pStr);
    }
    else
    {
      event_log_error (hashcat_ctx, "hipEventRecord(): %d", HIP_err);
    }

    return -1;
  }

  return 0;
}

int hc_hipEventSynchronize (void *hashcat_ctx, hipEvent_t hEvent)
{
  backend_ctx_t *backend_ctx = ((hashcat_ctx_t *) hashcat_ctx)->backend_ctx;

  HIP_PTR *hip = (HIP_PTR *) backend_ctx->hip;

  const hipError_t HIP_err = hip->hipEventSynchronize (hEvent);

  if (HIP_err != hipSuccess)
  {
    const char *pStr = NULL;

    if (hip->hipGetErrorString (HIP_err, &pStr) == hipSuccess)
    {
      event_log_error (hashcat_ctx, "hipEventSynchronize(): %s", pStr);
    }
    else
    {
      event_log_error (hashcat_ctx, "hipEventSynchronize(): %d", HIP_err);
    }

    return -1;
  }

  return 0;
}

int hc_hipFuncGetAttribute (void *hashcat_ctx, int *pi, hipFunction_attribute attrib, hipFunction_t hfunc)
{
  backend_ctx_t *backend_ctx = ((hashcat_ctx_t *) hashcat_ctx)->backend_ctx;

  HIP_PTR *hip = (HIP_PTR *) backend_ctx->hip;

  const hipError_t HIP_err = hip->hipFuncGetAttribute (pi, attrib, hfunc);

  if (HIP_err != hipSuccess)
  {
    const char *pStr = NULL;

    if (hip->hipGetErrorString (HIP_err, &pStr) == hipSuccess)
    {
      event_log_error (hashcat_ctx, "hipFuncGetAttribute(): %s", pStr);
    }
    else
    {
      event_log_error (hashcat_ctx, "hipFuncGetAttribute(): %d", HIP_err);
    }

    return -1;
  }

  return 0;
}

int hc_hipLaunchKernel (void *hashcat_ctx, hipFunction_t f, unsigned int gridDimX, unsigned int gridDimY, unsigned int gridDimZ, unsigned int blockDimX, unsigned int blockDimY, unsigned int blockDimZ, unsigned int sharedMemBytes, hipStream_t hStream, void **kernelParams, void **extra)
{
  backend_ctx_t *backend_ctx = ((hashcat_ctx_t *) hashcat_ctx)->backend_ctx;

  HIP_PTR *hip = (HIP_PTR *) backend_ctx->hip;

  const hipError_t HIP_err = hip->hipLaunchKernel (f, gridDimX, gridDimY, gridDimZ, blockDimX, blockDimY, blockDimZ, sharedMemBytes, hStream, kernelParams, extra);

  if (HIP_err != hipSuccess)
  {
    const char *pStr = NULL;

    if (hip->hipGetErrorString (HIP_err, &pStr) == hipSuccess)
    {
      event_log_error (hashcat_ctx, "hipLaunchKernel(): %s", pStr);
    }
    else
    {
      event_log_error (hashcat_ctx, "hipLaunchKernel(): %d", HIP_err);
    }

    return -1;
  }

  return 0;
}

int hc_hipInit (void *hashcat_ctx, unsigned int Flags)
{
  backend_ctx_t *backend_ctx = ((hashcat_ctx_t *) hashcat_ctx)->backend_ctx;

  HIP_PTR *hip = (HIP_PTR *) backend_ctx->hip;

  const hipError_t HIP_err = hip->hipInit (Flags);

  if (HIP_err != hipSuccess)
  {
    const char *pStr = NULL;

    if (hip->hipGetErrorString (HIP_err, &pStr) == hipSuccess)
    {
      event_log_error (hashcat_ctx, "hipInit(): %s", pStr);
    }
    else
    {
      event_log_error (hashcat_ctx, "hipInit(): %d", HIP_err);
    }

    return -1;
  }

  return 0;
}

int hc_hipMemAlloc (void *hashcat_ctx, hipDeviceptr_t *dptr, size_t bytesize)
{
  backend_ctx_t *backend_ctx = ((hashcat_ctx_t *) hashcat_ctx)->backend_ctx;

  HIP_PTR *hip = (HIP_PTR *) backend_ctx->hip;

  const hipError_t HIP_err = hip->hipMemAlloc (dptr, bytesize);

  if (HIP_err != hipSuccess)
  {
    const char *pStr = NULL;

    if (hip->hipGetErrorString (HIP_err, &pStr) == hipSuccess)
    {
      event_log_error (hashcat_ctx, "hipMemAlloc(): %s", pStr);
    }
    else
    {
      event_log_error (hashcat_ctx, "hipMemAlloc(): %d", HIP_err);
    }

    return -1;
  }

  return 0;
}

int hc_hipMemFree (void *hashcat_ctx, hipDeviceptr_t dptr)
{
  backend_ctx_t *backend_ctx = ((hashcat_ctx_t *) hashcat_ctx)->backend_ctx;

  HIP_PTR *hip = (HIP_PTR *) backend_ctx->hip;

  const hipError_t HIP_err = hip->hipMemFree (dptr);

  if (HIP_err != hipSuccess)
  {
    const char *pStr = NULL;

    if (hip->hipGetErrorString (HIP_err, &pStr) == hipSuccess)
    {
      event_log_error (hashcat_ctx, "hipMemFree(): %s", pStr);
    }
    else
    {
      event_log_error (hashcat_ctx, "hipMemFree(): %d", HIP_err);
    }

    return -1;
  }

  return 0;
}

int hc_hipMemGetInfo (void *hashcat_ctx, size_t *free, size_t *total)
{
  backend_ctx_t *backend_ctx = ((hashcat_ctx_t *) hashcat_ctx)->backend_ctx;

  HIP_PTR *hip = (HIP_PTR *) backend_ctx->hip;

  const hipError_t HIP_err = hip->hipMemGetInfo (free, total);

  if (HIP_err != hipSuccess)
  {
    const char *pStr = NULL;

    if (hip->hipGetErrorString (HIP_err, &pStr) == hipSuccess)
    {
      event_log_error (hashcat_ctx, "hipMemGetInfo(): %s", pStr);
    }
    else
    {
      event_log_error (hashcat_ctx, "hipMemGetInfo(): %d", HIP_err);
    }

    return -1;
  }

  return 0;
}



int hc_hipMemcpyDtoH (void *hashcat_ctx, void *dstHost, hipDeviceptr_t srcDevice, size_t ByteCount)
{
  backend_ctx_t *backend_ctx = ((hashcat_ctx_t *) hashcat_ctx)->backend_ctx;

  HIP_PTR *hip = (HIP_PTR *) backend_ctx->hip;

  const hipError_t HIP_err = hip->hipMemcpyDtoH (dstHost, srcDevice, ByteCount);

  if (HIP_err != hipSuccess)
  {
    const char *pStr = NULL;

    if (hip->hipGetErrorString (HIP_err, &pStr) == hipSuccess)
    {
      event_log_error (hashcat_ctx, "hipMemcpyDtoH(): %s", pStr);
    }
    else
    {
      event_log_error (hashcat_ctx, "hipMemcpyDtoH(): %d", HIP_err);
    }

    return -1;
  }

  return 0;
}

int hc_hipMemcpyDtoD (void *hashcat_ctx, hipDeviceptr_t dstDevice, hipDeviceptr_t srcDevice, size_t ByteCount)
{
  backend_ctx_t *backend_ctx = ((hashcat_ctx_t *) hashcat_ctx)->backend_ctx;

  HIP_PTR *hip = (HIP_PTR *) backend_ctx->hip;

  const hipError_t HIP_err = hip->hipMemcpyDtoD (dstDevice, srcDevice, ByteCount);

  if (HIP_err != hipSuccess)
  {
    const char *pStr = NULL;

    if (hip->hipGetErrorString (HIP_err, &pStr) == hipSuccess)
    {
      event_log_error (hashcat_ctx, "hipMemcpyDtoD(): %s", pStr);
    }
    else
    {
      event_log_error (hashcat_ctx, "hipMemcpyDtoD(): %d", HIP_err);
    }

    return -1;
  }

  return 0;
}

int hc_hipMemcpyHtoD (void *hashcat_ctx, hipDeviceptr_t dstDevice, const void *srcHost, size_t ByteCount)
{
  backend_ctx_t *backend_ctx = ((hashcat_ctx_t *) hashcat_ctx)->backend_ctx;

  HIP_PTR *hip = (HIP_PTR *) backend_ctx->hip;

  const hipError_t HIP_err = hip->hipMemcpyHtoD (dstDevice, srcHost, ByteCount);

  if (HIP_err != hipSuccess)
  {
    const char *pStr = NULL;

    if (hip->hipGetErrorString (HIP_err, &pStr) == hipSuccess)
    {
      event_log_error (hashcat_ctx, "hipMemcpyHtoD(): %s", pStr);
    }
    else
    {
      event_log_error (hashcat_ctx, "hipMemcpyHtoD(): %d", HIP_err);
    }

    return -1;
  }

  return 0;
}

int hc_hipMemsetD32 (void *hashcat_ctx, hipDeviceptr_t dstDevice, unsigned int ui, size_t N)
{
  backend_ctx_t *backend_ctx = ((hashcat_ctx_t *) hashcat_ctx)->backend_ctx;

  HIP_PTR *hip = (HIP_PTR *) backend_ctx->hip;

  const hipError_t HIP_err = hip->hipMemsetD32 (dstDevice, ui, N);

  if (HIP_err != hipSuccess)
  {
    const char *pStr = NULL;

    if (hip->hipGetErrorString (HIP_err, &pStr) == hipSuccess)
    {
      event_log_error (hashcat_ctx, "hipMemsetD32(): %s", pStr);
    }
    else
    {
      event_log_error (hashcat_ctx, "hipMemsetD32(): %d", HIP_err);
    }

    return -1;
  }

  return 0;
}

int hc_hipMemsetD8 (void *hashcat_ctx, hipDeviceptr_t dstDevice, unsigned char uc, size_t N)
{
  backend_ctx_t *backend_ctx = ((hashcat_ctx_t *) hashcat_ctx)->backend_ctx;

  HIP_PTR *hip = (HIP_PTR *) backend_ctx->hip;

  const hipError_t HIP_err = hip->hipMemsetD8 (dstDevice, uc, N);

  if (HIP_err != hipSuccess)
  {
    const char *pStr = NULL;

    if (hip->hipGetErrorString (HIP_err, &pStr) == hipSuccess)
    {
      event_log_error (hashcat_ctx, "hipMemsetD8(): %s", pStr);
    }
    else
    {
      event_log_error (hashcat_ctx, "hipMemsetD8(): %d", HIP_err);
    }

    return -1;
  }

  return 0;
}

int hc_hipMemcpyDtoHAsync (void *hashcat_ctx, void *dstHost, hipDeviceptr_t srcDevice, size_t ByteCount, hipStream_t hStream)
{
  backend_ctx_t *backend_ctx = ((hashcat_ctx_t *) hashcat_ctx)->backend_ctx;

  HIP_PTR *hip = (HIP_PTR *) backend_ctx->hip;

  const hipError_t HIP_err = hip->hipMemcpyDtoHAsync (dstHost, srcDevice, ByteCount, hStream);

  if (HIP_err != hipSuccess)
  {
    const char *pStr = NULL;

    if (hip->hipGetErrorString (HIP_err, &pStr) == hipSuccess)
    {
      event_log_error (hashcat_ctx, "hipMemcpyDtoHAsync(): %s", pStr);
    }
    else
    {
      event_log_error (hashcat_ctx, "hipMemcpyDtoHAsync(): %d", HIP_err);
    }

    return -1;
  }

  return 0;
}

int hc_hipMemcpyDtoDAsync (void *hashcat_ctx, hipDeviceptr_t dstDevice, hipDeviceptr_t srcDevice, size_t ByteCount, hipStream_t hStream)
{
  backend_ctx_t *backend_ctx = ((hashcat_ctx_t *) hashcat_ctx)->backend_ctx;

  HIP_PTR *hip = (HIP_PTR *) backend_ctx->hip;

  const hipError_t HIP_err = hip->hipMemcpyDtoDAsync (dstDevice, srcDevice, ByteCount, hStream);

  if (HIP_err != hipSuccess)
  {
    const char *pStr = NULL;

    if (hip->hipGetErrorString (HIP_err, &pStr) == hipSuccess)
    {
      event_log_error (hashcat_ctx, "hipMemcpyDtoDAsync(): %s", pStr);
    }
    else
    {
      event_log_error (hashcat_ctx, "hipMemcpyDtoDAsync(): %d", HIP_err);
    }

    return -1;
  }

  return 0;
}

int hc_hipMemcpyHtoDAsync (void *hashcat_ctx, hipDeviceptr_t dstDevice, const void *srcHost, size_t ByteCount, hipStream_t hStream)
{
  backend_ctx_t *backend_ctx = ((hashcat_ctx_t *) hashcat_ctx)->backend_ctx;

  HIP_PTR *hip = (HIP_PTR *) backend_ctx->hip;

  const hipError_t HIP_err = hip->hipMemcpyHtoDAsync (dstDevice, srcHost, ByteCount, hStream);

  if (HIP_err != hipSuccess)
  {
    const char *pStr = NULL;

    if (hip->hipGetErrorString (HIP_err, &pStr) == hipSuccess)
    {
      event_log_error (hashcat_ctx, "hipMemcpyHtoDAsync(): %s", pStr);
    }
    else
    {
      event_log_error (hashcat_ctx, "hipMemcpyHtoDAsync(): %d", HIP_err);
    }

    return -1;
  }

  return 0;
}

int hc_hipMemsetD32Async (void *hashcat_ctx, hipDeviceptr_t dstDevice, unsigned int ui, size_t N, hipStream_t hStream)
{
  backend_ctx_t *backend_ctx = ((hashcat_ctx_t *) hashcat_ctx)->backend_ctx;

  HIP_PTR *hip = (HIP_PTR *) backend_ctx->hip;

  const hipError_t HIP_err = hip->hipMemsetD32Async (dstDevice, ui, N, hStream);

  if (HIP_err != hipSuccess)
  {
    const char *pStr = NULL;

    if (hip->hipGetErrorString (HIP_err, &pStr) == hipSuccess)
    {
      event_log_error (hashcat_ctx, "hipMemsetD32Async(): %s", pStr);
    }
    else
    {
      event_log_error (hashcat_ctx, "hipMemsetD32Async(): %d", HIP_err);
    }

    return -1;
  }

  return 0;
}

int hc_hipMemsetD8Async (void *hashcat_ctx, hipDeviceptr_t dstDevice, unsigned char uc, size_t N, hipStream_t hStream)
{
  backend_ctx_t *backend_ctx = ((hashcat_ctx_t *) hashcat_ctx)->backend_ctx;

  HIP_PTR *hip = (HIP_PTR *) backend_ctx->hip;

  const hipError_t HIP_err = hip->hipMemsetD8Async (dstDevice, uc, N, hStream);

  if (HIP_err != hipSuccess)
  {
    const char *pStr = NULL;

    if (hip->hipGetErrorString (HIP_err, &pStr) == hipSuccess)
    {
      event_log_error (hashcat_ctx, "hipMemsetD8Async(): %s", pStr);
    }
    else
    {
      event_log_error (hashcat_ctx, "hipMemsetD8Async(): %d", HIP_err);
    }

    return -1;
  }

  return 0;
}

int hc_hipModuleGetFunction (void *hashcat_ctx, hipFunction_t *hfunc, hipModule_t hmod, const char *name)
{
  backend_ctx_t *backend_ctx = ((hashcat_ctx_t *) hashcat_ctx)->backend_ctx;

  HIP_PTR *hip = (HIP_PTR *) backend_ctx->hip;

  const hipError_t HIP_err = hip->hipModuleGetFunction (hfunc, hmod, name);

  if (HIP_err != hipSuccess)
  {
    const char *pStr = NULL;

    if (hip->hipGetErrorString (HIP_err, &pStr) == hipSuccess)
    {
      event_log_error (hashcat_ctx, "hipModuleGetFunction(): %s", pStr);
    }
    else
    {
      event_log_error (hashcat_ctx, "hipModuleGetFunction(): %d", HIP_err);
    }

    return -1;
  }

  return 0;
}

int hc_hipModuleGetGlobal (void *hashcat_ctx, hipDeviceptr_t *dptr, size_t *bytes, hipModule_t hmod, const char *name)
{
  backend_ctx_t *backend_ctx = ((hashcat_ctx_t *) hashcat_ctx)->backend_ctx;

  HIP_PTR *hip = (HIP_PTR *) backend_ctx->hip;

  const hipError_t HIP_err = hip->hipModuleGetGlobal (dptr, bytes, hmod, name);

  if (HIP_err != hipSuccess)
  {
    const char *pStr = NULL;

    if (hip->hipGetErrorString (HIP_err, &pStr) == hipSuccess)
    {
      event_log_error (hashcat_ctx, "hipModuleGetGlobal(): %s", pStr);
    }
    else
    {
      event_log_error (hashcat_ctx, "hipModuleGetGlobal(): %d", HIP_err);
    }

    return -1;
  }

  return 0;
}

int hc_hipModuleLoadDataEx (void *hashcat_ctx, hipModule_t *module, const void *image, unsigned int numOptions, hipJitOption *options, void **optionValues)
{
  backend_ctx_t *backend_ctx = ((hashcat_ctx_t *) hashcat_ctx)->backend_ctx;

  HIP_PTR *hip = (HIP_PTR *) backend_ctx->hip;

  const hipError_t HIP_err = hip->hipModuleLoadDataEx (module, image, numOptions, options, optionValues);

  if (HIP_err != hipSuccess)
  {
    const char *pStr = NULL;

    if (hip->hipGetErrorString (HIP_err, &pStr) == hipSuccess)
    {
      event_log_error (hashcat_ctx, "hipModuleLoadDataEx(): %s", pStr);
    }
    else
    {
      event_log_error (hashcat_ctx, "hipModuleLoadDataEx(): %d", HIP_err);
    }

    return -1;
  }

  return 0;
}

int hc_hipModuleUnload (void *hashcat_ctx, hipModule_t hmod)
{
  backend_ctx_t *backend_ctx = ((hashcat_ctx_t *) hashcat_ctx)->backend_ctx;

  HIP_PTR *hip = (HIP_PTR *) backend_ctx->hip;

  const hipError_t HIP_err = hip->hipModuleUnload (hmod);

  if (HIP_err != hipSuccess)
  {
    const char *pStr = NULL;

    if (hip->hipGetErrorString (HIP_err, &pStr) == hipSuccess)
    {
      event_log_error (hashcat_ctx, "hipModuleUnload(): %s", pStr);
    }
    else
    {
      event_log_error (hashcat_ctx, "hipModuleUnload(): %d", HIP_err);
    }

    return -1;
  }

  return 0;
}

int hc_hipRuntimeGetVersion (void *hashcat_ctx, int *runtimeVersion)
{
  backend_ctx_t *backend_ctx = ((hashcat_ctx_t *) hashcat_ctx)->backend_ctx;

  HIP_PTR *hip = (HIP_PTR *) backend_ctx->hip;

  const hipError_t HIP_err = hip->hipRuntimeGetVersion (runtimeVersion);

  if (HIP_err != hipSuccess)
  {
    const char *pStr = NULL;

    if (hip->hipGetErrorString (HIP_err, &pStr) == hipSuccess)
    {
      event_log_error (hashcat_ctx, "hipRuntimeGetVersion(): %s", pStr);
    }
    else
    {
      event_log_error (hashcat_ctx, "hipRuntimeGetVersion(): %d", HIP_err);
    }

    return -1;
  }

  return 0;
}

int hc_hipSetDevice (void *hashcat_ctx, hipDevice_t dev)
{
  backend_ctx_t *backend_ctx = ((hashcat_ctx_t *) hashcat_ctx)->backend_ctx;

  HIP_PTR *hip = (HIP_PTR *) backend_ctx->hip;

  const hipError_t HIP_err = hip->hipSetDevice (dev);

  if (HIP_err != hipSuccess)
  {
    const char *pStr = NULL;

    if (hip->hipGetErrorString (HIP_err, &pStr) == hipSuccess)
    {
      event_log_error (hashcat_ctx, "hipSetDevice(): %s", pStr);
    }
    else
    {
      event_log_error (hashcat_ctx, "hipSetDevice(): %d", HIP_err);
    }

    return -1;
  }

  return 0;
}

int hc_hipSetDeviceFlags (void *hashcat_ctx, unsigned int flags)
{
  backend_ctx_t *backend_ctx = ((hashcat_ctx_t *) hashcat_ctx)->backend_ctx;

  HIP_PTR *hip = (HIP_PTR *) backend_ctx->hip;

  const hipError_t HIP_err = hip->hipSetDeviceFlags (flags);

  if (HIP_err != hipSuccess)
  {
    const char *pStr = NULL;

    if (hip->hipGetErrorString (HIP_err, &pStr) == hipSuccess)
    {
      event_log_error (hashcat_ctx, "hipSetDeviceFlags(): %s", pStr);
    }
    else
    {
      event_log_error (hashcat_ctx, "hipSetDeviceFlags(): %d", HIP_err);
    }

    return -1;
  }

  return 0;
}

int hc_hipStreamCreate (void *hashcat_ctx, hipStream_t *phStream)
{
  backend_ctx_t *backend_ctx = ((hashcat_ctx_t *) hashcat_ctx)->backend_ctx;

  HIP_PTR *hip = (HIP_PTR *) backend_ctx->hip;

  const hipError_t HIP_err = hip->hipStreamCreate (phStream);

  if (HIP_err != hipSuccess)
  {
    const char *pStr = NULL;

    if (hip->hipGetErrorString (HIP_err, &pStr) == hipSuccess)
    {
      event_log_error (hashcat_ctx, "hipStreamCreate(): %s", pStr);
    }
    else
    {
      event_log_error (hashcat_ctx, "hipStreamCreate(): %d", HIP_err);
    }

    return -1;
  }

  return 0;
}

int hc_hipStreamCreateWithFlags (void *hashcat_ctx, hipStream_t *phStream, unsigned int Flags)
{
  backend_ctx_t *backend_ctx = ((hashcat_ctx_t *) hashcat_ctx)->backend_ctx;

  HIP_PTR *hip = (HIP_PTR *) backend_ctx->hip;

  const hipError_t HIP_err = hip->hipStreamCreateWithFlags (phStream, Flags);

  if (HIP_err != hipSuccess)
  {
    const char *pStr = NULL;

    if (hip->hipGetErrorString (HIP_err, &pStr) == hipSuccess)
    {
      event_log_error (hashcat_ctx, "hipStreamCreateWithFlags(): %s", pStr);
    }
    else
    {
      event_log_error (hashcat_ctx, "hipStreamCreateWithFlags(): %d", HIP_err);
    }

    return -1;
  }

  return 0;
}

int hc_hipStreamDestroy (void *hashcat_ctx, hipStream_t hStream)
{
  backend_ctx_t *backend_ctx = ((hashcat_ctx_t *) hashcat_ctx)->backend_ctx;

  HIP_PTR *hip = (HIP_PTR *) backend_ctx->hip;

  const hipError_t HIP_err = hip->hipStreamDestroy (hStream);

  if (HIP_err != hipSuccess)
  {
    const char *pStr = NULL;

    if (hip->hipGetErrorString (HIP_err, &pStr) == hipSuccess)
    {
      event_log_error (hashcat_ctx, "hipStreamDestroy(): %s", pStr);
    }
    else
    {
      event_log_error (hashcat_ctx, "hipStreamDestroy(): %d", HIP_err);
    }

    return -1;
  }

  return 0;
}

int hc_hipStreamSynchronize (void *hashcat_ctx, hipStream_t hStream)
{
  backend_ctx_t *backend_ctx = ((hashcat_ctx_t *) hashcat_ctx)->backend_ctx;

  HIP_PTR *hip = (HIP_PTR *) backend_ctx->hip;

  const hipError_t HIP_err = hip->hipStreamSynchronize (hStream);

  if (HIP_err != hipSuccess)
  {
    const char *pStr = NULL;

    if (hip->hipGetErrorString (HIP_err, &pStr) == hipSuccess)
    {
      event_log_error (hashcat_ctx, "hipStreamSynchronize(): %s", pStr);
    }
    else
    {
      event_log_error (hashcat_ctx, "hipStreamSynchronize(): %d", HIP_err);
    }

    return -1;
  }

  return 0;
}

int hc_hipGetDeviceProperties (void *hashcat_ctx, hipDeviceProp_t *prop, hipDevice_t dev)
{
  backend_ctx_t *backend_ctx = ((hashcat_ctx_t *) hashcat_ctx)->backend_ctx;

  HIP_PTR *hip = (HIP_PTR *) backend_ctx->hip;

  const hipError_t HIP_err = hip->hipGetDeviceProperties (prop, dev);

  if (HIP_err != hipSuccess)
  {
    const char *pStr = NULL;

    if (hip->hipGetErrorString (HIP_err, &pStr) == hipSuccess)
    {
      event_log_error (hashcat_ctx, "hipDeviceGetAttribute(): %s", pStr);
    }
    else
    {
      event_log_error (hashcat_ctx, "hipDeviceGetAttribute(): %d", HIP_err);
    }

    return -1;
  }

  return 0;
}

int hc_hipModuleOccupancyMaxActiveBlocksPerMultiprocessor (void *hashcat_ctx, int *numBlocks, hipFunction_t f, int blockSize, size_t dynSharedMemPerBlk)
{
  backend_ctx_t *backend_ctx = ((hashcat_ctx_t *) hashcat_ctx)->backend_ctx;

  HIP_PTR *hip = (HIP_PTR *) backend_ctx->hip;

  const hipError_t HIP_err = hip->hipModuleOccupancyMaxActiveBlocksPerMultiprocessor (numBlocks, f, blockSize, dynSharedMemPerBlk);

  if (HIP_err != hipSuccess)
  {
    const char *pStr = NULL;

    if (hip->hipGetErrorString (HIP_err, &pStr) == hipSuccess)
    {
      event_log_error (hashcat_ctx, "hipModuleOccupancyMaxActiveBlocksPerMultiprocessor(): %s", pStr);
    }
    else
    {
      event_log_error (hashcat_ctx, "hipModuleOccupancyMaxActiveBlocksPerMultiprocessor(): %d", HIP_err);
    }

    return -1;
  }

  return 0;
}
