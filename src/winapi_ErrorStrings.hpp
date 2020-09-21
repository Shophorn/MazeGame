#if !defined HACKED_WIN_ERRORS_TO_STRING_HPP

#include <string>
#include <sstream>

template<typename ValueType>
static std::string
UnsupportedValue(const char * messge, ValueType value)
{
    std::stringstream ss;
    ss << messge << ": " << value;
    return ss.str();
}

internal char const * 
WinApiWaveFormatTagString(WORD value)
{
    switch (value)
    {
        case WAVE_FORMAT_PCM:         return "WAVE_FORMAT_PCM";
        case WAVE_FORMAT_IEEE_FLOAT:  return "WAVE_FORMAT_IEEE_FLOAT";
        case WAVE_FORMAT_EXTENSIBLE:  return "WAVE_FORMAT_EXTENSIBLE";

        default:
            return "Unsupported format tag";
    }
}
    
internal char const * WinApiErrorString(long long error)
{
    switch(error)
    {
        case S_OK:                                          return "S_OK";
        case REGDB_E_CLASSNOTREG:                           return "REGDB_E_CLASSNOTREG";
        case CLASS_E_NOAGGREGATION:                         return "CLASS_E_NOAGGREGATION";

        case E_NOINTERFACE:  return "E_NOINTERFACE";
        case E_PENDING:      return "E_PENDING";
        case E_HANDLE:       return "E_HANDLE";
        case E_NOTIMPL:      return "E_NOTIMPL";
        case E_OUTOFMEMORY:  return "E_OUTOFMEMORY";
        case E_UNEXPECTED:   return "E_UNEXPECTED";
        case E_ACCESSDENIED: return "E_ACCESSDENIED";
        case E_POINTER:      return "E_POINTER";
        case E_ABORT:        return "E_ABORT";
        case E_INVALIDARG:   return "E_INVALIDARG";
        case E_FAIL:         return "E_FAIL";

        case RPC_E_SYS_CALL_FAILED:              return "RPC_E_SYS_CALL_FAILED";
        case RPC_E_CONNECTION_TERMINATED:        return "RPC_E_CONNECTION_TERMINATED";
        case RPC_E_CALL_COMPLETE:                return "RPC_E_CALL_COMPLETE";
        case RPC_E_FAULT:                        return "RPC_E_FAULT";
        case RPC_E_CALL_CANCELED:                return "RPC_E_CALL_CANCELED";
        case RPC_E_RETRY:                        return "RPC_E_RETRY";
        case RPC_E_SERVERCALL_REJECTED:          return "RPC_E_SERVERCALL_REJECTED";
        case RPC_E_CALL_REJECTED:                return "RPC_E_CALL_REJECTED";
        case RPC_E_CLIENT_CANTUNMARSHAL_DATA:    return "RPC_E_CLIENT_CANTUNMARSHAL_DATA";
        case RPC_E_CANTTRANSMIT_CALL:            return "RPC_E_CANTTRANSMIT_CALL";
        case RPC_E_SERVERCALL_RETRYLATER:        return "RPC_E_SERVERCALL_RETRYLATER";
        case RPC_E_CANTCALLOUT_INASYNCCALL:      return "RPC_E_CANTCALLOUT_INASYNCCALL";
        case RPC_E_VERSION_MISMATCH:             return "RPC_E_VERSION_MISMATCH";
        case RPC_E_SERVER_CANTMARSHAL_DATA:      return "RPC_E_SERVER_CANTMARSHAL_DATA";
        case RPC_E_WRONG_THREAD:                 return "RPC_E_WRONG_THREAD";
        case RPC_E_INVALID_CALLDATA:             return "RPC_E_INVALID_CALLDATA";
        case RPC_E_CANTCALLOUT_ININPUTSYNCCALL:  return "RPC_E_CANTCALLOUT_ININPUTSYNCCALL";
        case RPC_E_CANTCALLOUT_AGAIN:            return "RPC_E_CANTCALLOUT_AGAIN";
        case RPC_E_UNSECURE_CALL:                return "RPC_E_UNSECURE_CALL";
        case RPC_E_NO_CONTEXT:                   return "RPC_E_NO_CONTEXT";
        case RPC_E_OUT_OF_RESOURCES:             return "RPC_E_OUT_OF_RESOURCES";
        case RPC_E_CHANGED_MODE:                 return "RPC_E_CHANGED_MODE";
        case RPC_E_ATTEMPTED_MULTITHREAD:        return "RPC_E_ATTEMPTED_MULTITHREAD";
        case RPC_E_INVALID_DATAPACKET:           return "RPC_E_INVALID_DATAPACKET";
        case RPC_E_DISCONNECTED:                 return "RPC_E_DISCONNECTED";
        case RPC_E_INVALID_OBJECT:               return "RPC_E_INVALID_OBJECT";
        case RPC_E_NOT_REGISTERED:               return "RPC_E_NOT_REGISTERED";
        case RPC_E_REMOTE_DISABLED:              return "RPC_E_REMOTE_DISABLED";
        case RPC_E_NO_SYNC:                      return "RPC_E_NO_SYNC";
        case RPC_E_INVALID_PARAMETER:            return "RPC_E_INVALID_PARAMETER";
        case RPC_E_TOO_LATE:                     return "RPC_E_TOO_LATE";
        case RPC_E_INVALID_STD_NAME:             return "RPC_E_INVALID_STD_NAME";
        case RPC_E_INVALID_EXTENSION:            return "RPC_E_INVALID_EXTENSION";
        case RPC_E_SERVER_DIED:                  return "RPC_E_SERVER_DIED";
        case RPC_E_CANTPOST_INSENDCALL:          return "RPC_E_CANTPOST_INSENDCALL";
        case RPC_E_INVALID_HEADER:               return "RPC_E_INVALID_HEADER";
        case RPC_E_INVALID_OBJREF:               return "RPC_E_INVALID_OBJREF";
        case RPC_E_UNEXPECTED:                   return "RPC_E_UNEXPECTED";
        case RPC_E_FULLSIC_REQUIRED:             return "RPC_E_FULLSIC_REQUIRED";
        case RPC_E_SERVER_DIED_DNE:              return "RPC_E_SERVER_DIED_DNE";
        case RPC_E_INVALID_DATA:                 return "RPC_E_INVALID_DATA";
        case RPC_E_CANTCALLOUT_INEXTERNALCALL:   return "RPC_E_CANTCALLOUT_INEXTERNALCALL";
        case RPC_E_INVALID_IPID:                 return "RPC_E_INVALID_IPID";
        case RPC_E_CLIENT_CANTMARSHAL_DATA:      return "RPC_E_CLIENT_CANTMARSHAL_DATA";
        case RPC_E_NO_GOOD_SECURITY_PACKAGES:    return "RPC_E_NO_GOOD_SECURITY_PACKAGES";
        case RPC_E_ACCESS_DENIED:                return "RPC_E_ACCESS_DENIED";
        case RPC_E_SERVERFAULT:                  return "RPC_E_SERVERFAULT";
        case RPC_E_TIMEOUT:                      return "RPC_E_TIMEOUT";
        case RPC_E_INVALIDMETHOD:                return "RPC_E_INVALIDMETHOD";
        case RPC_E_CLIENT_DIED:                  return "RPC_E_CLIENT_DIED";
        case RPC_E_SERVER_CANTUNMARSHAL_DATA:    return "RPC_E_SERVER_CANTUNMARSHAL_DATA";
        case RPC_E_THREAD_NOT_INIT:              return "RPC_E_THREAD_NOT_INIT";

        case CO_E_INIT_TLS:                                  return "CO_E_INIT_TLS";
        case CO_E_INIT_SHARED_ALLOCATOR:                     return "CO_E_INIT_SHARED_ALLOCATOR";
        case CO_E_INIT_MEMORY_ALLOCATOR:                     return "CO_E_INIT_MEMORY_ALLOCATOR";
        case CO_E_INIT_CLASS_CACHE:                          return "CO_E_INIT_CLASS_CACHE";
        case CO_E_INIT_RPC_CHANNEL:                          return "CO_E_INIT_RPC_CHANNEL";
        case CO_E_INIT_TLS_SET_CHANNEL_CONTROL:              return "CO_E_INIT_TLS_SET_CHANNEL_CONTROL";
        case CO_E_INIT_TLS_CHANNEL_CONTROL:                  return "CO_E_INIT_TLS_CHANNEL_CONTROL";
        case CO_E_INIT_UNACCEPTED_USER_ALLOCATOR:            return "CO_E_INIT_UNACCEPTED_USER_ALLOCATOR";
        case CO_E_INIT_SCM_MUTEX_EXISTS:                     return "CO_E_INIT_SCM_MUTEX_EXISTS";
        case CO_E_INIT_SCM_FILE_MAPPING_EXISTS:              return "CO_E_INIT_SCM_FILE_MAPPING_EXISTS";
        case CO_E_INIT_SCM_MAP_VIEW_OF_FILE:                 return "CO_E_INIT_SCM_MAP_VIEW_OF_FILE";
        case CO_E_INIT_SCM_EXEC_FAILURE:                     return "CO_E_INIT_SCM_EXEC_FAILURE";
        case CO_E_INIT_ONLY_SINGLE_THREADED:                 return "CO_E_INIT_ONLY_SINGLE_THREADED";
        case CO_E_CANT_REMOTE:                               return "CO_E_CANT_REMOTE";
        case CO_E_BAD_SERVER_NAME:                           return "CO_E_BAD_SERVER_NAME";
        case CO_E_WRONG_SERVER_IDENTITY:                     return "CO_E_WRONG_SERVER_IDENTITY";
        case CO_E_OLE1DDE_DISABLED:                          return "CO_E_OLE1DDE_DISABLED";
        case CO_E_RUNAS_SYNTAX:                              return "CO_E_RUNAS_SYNTAX";
        case CO_E_CREATEPROCESS_FAILURE:                     return "CO_E_CREATEPROCESS_FAILURE";
        case CO_E_RUNAS_CREATEPROCESS_FAILURE:               return "CO_E_RUNAS_CREATEPROCESS_FAILURE";
        case CO_E_RUNAS_LOGON_FAILURE:                       return "CO_E_RUNAS_LOGON_FAILURE";
        case CO_E_LAUNCH_PERMSSION_DENIED:                   return "CO_E_LAUNCH_PERMSSION_DENIED";
        case CO_E_START_SERVICE_FAILURE:                     return "CO_E_START_SERVICE_FAILURE";
        case CO_E_REMOTE_COMMUNICATION_FAILURE:              return "CO_E_REMOTE_COMMUNICATION_FAILURE";
        case CO_E_SERVER_START_TIMEOUT:                      return "CO_E_SERVER_START_TIMEOUT";
        case CO_E_CLSREG_INCONSISTENT:                       return "CO_E_CLSREG_INCONSISTENT";
        case CO_E_IIDREG_INCONSISTENT:                       return "CO_E_IIDREG_INCONSISTENT";
        case CO_E_NOT_SUPPORTED:                             return "CO_E_NOT_SUPPORTED";
        case CO_E_RELOAD_DLL:                                return "CO_E_RELOAD_DLL";
        case CO_E_MSI_ERROR:                                 return "CO_E_MSI_ERROR";
        case CO_E_ATTEMPT_TO_CREATE_OUTSIDE_CLIENT_CONTEXT:  return "CO_E_ATTEMPT_TO_CREATE_OUTSIDE_CLIENT_CONTEXT";
        case CO_E_SERVER_PAUSED:                             return "CO_E_SERVER_PAUSED";
        case CO_E_SERVER_NOT_PAUSED:                         return "CO_E_SERVER_NOT_PAUSED";
        case CO_E_CLASS_DISABLED:                            return "CO_E_CLASS_DISABLED";
        case CO_E_CLRNOTAVAILABLE:                           return "CO_E_CLRNOTAVAILABLE";
        case CO_E_ASYNC_WORK_REJECTED:                       return "CO_E_ASYNC_WORK_REJECTED";
        case CO_E_SERVER_INIT_TIMEOUT:                       return "CO_E_SERVER_INIT_TIMEOUT";
        case CO_E_NO_SECCTX_IN_ACTIVATE:                     return "CO_E_NO_SECCTX_IN_ACTIVATE";
        case CO_E_TRACKER_CONFIG:                            return "CO_E_TRACKER_CONFIG";
        case CO_E_THREADPOOL_CONFIG:                         return "CO_E_THREADPOOL_CONFIG";
        case CO_E_SXS_CONFIG:                                return "CO_E_SXS_CONFIG";
        case CO_E_MALFORMED_SPN:                             return "CO_E_MALFORMED_SPN";
        case CO_E_FIRST:                                     return "CO_E_FIRST";
        case CO_E_LAST:                                      return "CO_E_LAST";
        case CO_E_NOTINITIALIZED:                            return "CO_E_NOTINITIALIZED";
        case CO_E_ALREADYINITIALIZED:                        return "CO_E_ALREADYINITIALIZED";
        case CO_E_CANTDETERMINECLASS:                        return "CO_E_CANTDETERMINECLASS";
        case CO_E_CLASSSTRING:                               return "CO_E_CLASSSTRING";
        case CO_E_IIDSTRING:                                 return "CO_E_IIDSTRING";
        case CO_E_APPNOTFOUND:                               return "CO_E_APPNOTFOUND";
        case CO_E_APPSINGLEUSE:                              return "CO_E_APPSINGLEUSE";
        case CO_E_ERRORINAPP:                                return "CO_E_ERRORINAPP";
        case CO_E_DLLNOTFOUND:                               return "CO_E_DLLNOTFOUND";
        case CO_E_ERRORINDLL:                                return "CO_E_ERRORINDLL";
        case CO_E_WRONGOSFORAPP:                             return "CO_E_WRONGOSFORAPP";
        case CO_E_OBJNOTREG:                                 return "CO_E_OBJNOTREG";
        case CO_E_OBJISREG:                                  return "CO_E_OBJISREG";
        case CO_E_OBJNOTCONNECTED:                           return "CO_E_OBJNOTCONNECTED";
        case CO_E_APPDIDNTREG:                               return "CO_E_APPDIDNTREG";
        case CO_E_RELEASED:                                  return "CO_E_RELEASED";
        case CO_E_ACTIVATIONFAILED:                          return "CO_E_ACTIVATIONFAILED";
        case CO_E_ACTIVATIONFAILED_EVENTLOGGED:              return "CO_E_ACTIVATIONFAILED_EVENTLOGGED";
        case CO_E_ACTIVATIONFAILED_CATALOGERROR:             return "CO_E_ACTIVATIONFAILED_CATALOGERROR";
        case CO_E_ACTIVATIONFAILED_TIMEOUT:                  return "CO_E_ACTIVATIONFAILED_TIMEOUT";
        case CO_E_INITIALIZATIONFAILED:                      return "CO_E_INITIALIZATIONFAILED";
        case CO_E_THREADINGMODEL_CHANGED:                    return "CO_E_THREADINGMODEL_CHANGED";
        case CO_E_NOIISINTRINSICS:                           return "CO_E_NOIISINTRINSICS";
        case CO_E_NOCOOKIES:                                 return "CO_E_NOCOOKIES";
        case CO_E_DBERROR:                                   return "CO_E_DBERROR";
        case CO_E_NOTPOOLED:                                 return "CO_E_NOTPOOLED";
        case CO_E_NOTCONSTRUCTED:                            return "CO_E_NOTCONSTRUCTED";
        case CO_E_NOSYNCHRONIZATION:                         return "CO_E_NOSYNCHRONIZATION";
        case CO_E_ISOLEVELMISMATCH:                          return "CO_E_ISOLEVELMISMATCH";
        case CO_E_CLASS_CREATE_FAILED:                       return "CO_E_CLASS_CREATE_FAILED";
        case CO_E_SCM_ERROR:                                 return "CO_E_SCM_ERROR";
        case CO_E_SCM_RPC_FAILURE:                           return "CO_E_SCM_RPC_FAILURE";
        case CO_E_BAD_PATH:                                  return "CO_E_BAD_PATH";
        case CO_E_SERVER_EXEC_FAILURE:                       return "CO_E_SERVER_EXEC_FAILURE";
        case CO_E_OBJSRV_RPC_FAILURE:                        return "CO_E_OBJSRV_RPC_FAILURE";
        case CO_E_SERVER_STOPPING:                           return "CO_E_SERVER_STOPPING";
        case CO_E_FAILEDTOIMPERSONATE:                       return "CO_E_FAILEDTOIMPERSONATE";
        case CO_E_FAILEDTOGETSECCTX:                         return "CO_E_FAILEDTOGETSECCTX";
        case CO_E_FAILEDTOOPENTHREADTOKEN:                   return "CO_E_FAILEDTOOPENTHREADTOKEN";
        case CO_E_FAILEDTOGETTOKENINFO:                      return "CO_E_FAILEDTOGETTOKENINFO";
        case CO_E_TRUSTEEDOESNTMATCHCLIENT:                  return "CO_E_TRUSTEEDOESNTMATCHCLIENT";
        case CO_E_FAILEDTOQUERYCLIENTBLANKET:                return "CO_E_FAILEDTOQUERYCLIENTBLANKET";
        case CO_E_FAILEDTOSETDACL:                           return "CO_E_FAILEDTOSETDACL";
        case CO_E_ACCESSCHECKFAILED:                         return "CO_E_ACCESSCHECKFAILED";
        case CO_E_NETACCESSAPIFAILED:                        return "CO_E_NETACCESSAPIFAILED";
        case CO_E_WRONGTRUSTEENAMESYNTAX:                    return "CO_E_WRONGTRUSTEENAMESYNTAX";
        case CO_E_INVALIDSID:                                return "CO_E_INVALIDSID";
        case CO_E_CONVERSIONFAILED:                          return "CO_E_CONVERSIONFAILED";
        case CO_E_NOMATCHINGSIDFOUND:                        return "CO_E_NOMATCHINGSIDFOUND";
        case CO_E_LOOKUPACCSIDFAILED:                        return "CO_E_LOOKUPACCSIDFAILED";
        case CO_E_NOMATCHINGNAMEFOUND:                       return "CO_E_NOMATCHINGNAMEFOUND";
        case CO_E_LOOKUPACCNAMEFAILED:                       return "CO_E_LOOKUPACCNAMEFAILED";
        case CO_E_SETSERLHNDLFAILED:                         return "CO_E_SETSERLHNDLFAILED";
        case CO_E_FAILEDTOGETWINDIR:                         return "CO_E_FAILEDTOGETWINDIR";
        case CO_E_PATHTOOLONG:                               return "CO_E_PATHTOOLONG";
        case CO_E_FAILEDTOGENUUID:                           return "CO_E_FAILEDTOGENUUID";
        case CO_E_FAILEDTOCREATEFILE:                        return "CO_E_FAILEDTOCREATEFILE";
        case CO_E_FAILEDTOCLOSEHANDLE:                       return "CO_E_FAILEDTOCLOSEHANDLE";
        case CO_E_EXCEEDSYSACLLIMIT:                         return "CO_E_EXCEEDSYSACLLIMIT";
        case CO_E_ACESINWRONGORDER:                          return "CO_E_ACESINWRONGORDER";
        case CO_E_INCOMPATIBLESTREAMVERSION:                 return "CO_E_INCOMPATIBLESTREAMVERSION";
        case CO_E_FAILEDTOOPENPROCESSTOKEN:                  return "CO_E_FAILEDTOOPENPROCESSTOKEN";
        case CO_E_DECODEFAILED:                              return "CO_E_DECODEFAILED";
        case CO_E_ACNOTINITIALIZED:                          return "CO_E_ACNOTINITIALIZED";
        case CO_E_CANCEL_DISABLED:                           return "CO_E_CANCEL_DISABLED";


        case AUDCLNT_STREAMFLAGS_EVENTCALLBACK:              return "AUDCLNT_STREAMFLAGS_EVENTCALLBACK";
        case AUDCLNT_E_DEVICE_INVALIDATED:                   return "AUDCLNT_E_DEVICE_INVALIDATED";
        case AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED:           return "AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED";
        case AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED:              return "AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED";
        case AUDCLNT_SHAREMODE_EXCLUSIVE:                    return "AUDCLNT_SHAREMODE_EXCLUSIVE";
        case AUDCLNT_E_ALREADY_INITIALIZED:                  return "AUDCLNT_E_ALREADY_INITIALIZED";
        case AUDCLNT_E_BUFDURATION_PERIOD_NOT_EQUAL:         return "AUDCLNT_E_BUFDURATION_PERIOD_NOT_EQUAL";
        case AUDCLNT_E_SERVICE_NOT_RUNNING:                  return "AUDCLNT_E_SERVICE_NOT_RUNNING";
        case AUDCLNT_STREAMFLAGS_LOOPBACK:                   return "AUDCLNT_STREAMFLAGS_LOOPBACK";
        case AUDCLNT_E_WRONG_ENDPOINT_TYPE:                  return "AUDCLNT_E_WRONG_ENDPOINT_TYPE";
        case AUDCLNT_E_CPUUSAGE_EXCEEDED:                    return "AUDCLNT_E_CPUUSAGE_EXCEEDED";
        case AUDCLNT_E_INVALID_DEVICE_PERIOD:                return "AUDCLNT_E_INVALID_DEVICE_PERIOD";
        case AUDCLNT_E_UNSUPPORTED_FORMAT:                   return "AUDCLNT_E_UNSUPPORTED_FORMAT";
        case AUDCLNT_E_DEVICE_IN_USE:                        return "AUDCLNT_E_DEVICE_IN_USE";
        case AUDCLNT_E_BUFFER_SIZE_ERROR:                    return "AUDCLNT_E_BUFFER_SIZE_ERROR";
        case AUDCLNT_E_ENDPOINT_CREATE_FAILED:               return "AUDCLNT_E_ENDPOINT_CREATE_FAILED";

   
        default:
            return "Unsupported Error";
    }

}

#define HACKED_WIN_ERRORS_TO_STRING_HPP
#endif