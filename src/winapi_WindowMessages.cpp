namespace winapi
{
	const char * WindowMessageString(uint32 message);
}

// https://wiki.winehq.org/List_Of_Windows_Messages
const char *
winapi::WindowMessageString(uint32 message)
{
	switch(message)
	{
		case WM_NULL:                       return "WM_NULL";
		case WM_CREATE:                     return "WM_CREATE";
		case WM_DESTROY:                    return "WM_DESTROY";
		case WM_MOVE:                       return "WM_MOVE";
		case WM_SIZE:                       return "WM_SIZE";
		case WM_ACTIVATE:                   return "WM_ACTIVATE";
		case WM_SETFOCUS:                   return "WM_SETFOCUS";
		case WM_KILLFOCUS:                  return "WM_KILLFOCUS";
		case WM_ENABLE:                     return "WM_ENABLE";
		case WM_SETREDRAW:                  return "WM_SETREDRAW";
		case WM_SETTEXT:                    return "WM_SETTEXT";
		case WM_GETTEXT:                    return "WM_GETTEXT";
		case WM_GETTEXTLENGTH:              return "WM_GETTEXTLENGTH";
		case WM_PAINT:                      return "WM_PAINT";
		case WM_CLOSE:                      return "WM_CLOSE";
		case WM_QUERYENDSESSION:            return "WM_QUERYENDSESSION";
		case WM_QUIT:                       return "WM_QUIT";
		case WM_QUERYOPEN:                  return "WM_QUERYOPEN";
		case WM_ERASEBKGND:                 return "WM_ERASEBKGND";
		case WM_SYSCOLORCHANGE:             return "WM_SYSCOLORCHANGE";
		case WM_ENDSESSION:                 return "WM_ENDSESSION";
		case WM_SHOWWINDOW:                 return "WM_SHOWWINDOW";
		// case WM_CTLCOLOR:                   return "WM_CTLCOLOR";
		case WM_WININICHANGE:               return "WM_WININICHANGE";
		case WM_DEVMODECHANGE:              return "WM_DEVMODECHANGE";
		case WM_ACTIVATEAPP:                return "WM_ACTIVATEAPP";
		case WM_FONTCHANGE:                 return "WM_FONTCHANGE";
		case WM_TIMECHANGE:                 return "WM_TIMECHANGE";
		case WM_CANCELMODE:                 return "WM_CANCELMODE";
		case WM_SETCURSOR:                  return "WM_SETCURSOR";
		case WM_MOUSEACTIVATE:              return "WM_MOUSEACTIVATE";
		case WM_CHILDACTIVATE:              return "WM_CHILDACTIVATE";
		case WM_QUEUESYNC:                  return "WM_QUEUESYNC";
		case WM_GETMINMAXINFO:              return "WM_GETMINMAXINFO";
		case WM_PAINTICON:                  return "WM_PAINTICON";
		case WM_ICONERASEBKGND:             return "WM_ICONERASEBKGND";
		case WM_NEXTDLGCTL:                 return "WM_NEXTDLGCTL";
		case WM_SPOOLERSTATUS:              return "WM_SPOOLERSTATUS";
		case WM_DRAWITEM:                   return "WM_DRAWITEM";
		case WM_MEASUREITEM:                return "WM_MEASUREITEM";
		case WM_DELETEITEM:                 return "WM_DELETEITEM";
		case WM_VKEYTOITEM:                 return "WM_VKEYTOITEM";
		case WM_CHARTOITEM:                 return "WM_CHARTOITEM";
		case WM_SETFONT:                    return "WM_SETFONT";
		case WM_GETFONT:                    return "WM_GETFONT";
		case WM_SETHOTKEY:                  return "WM_SETHOTKEY";
		case WM_GETHOTKEY:                  return "WM_GETHOTKEY";
		case WM_QUERYDRAGICON:              return "WM_QUERYDRAGICON";
		case WM_COMPAREITEM:                return "WM_COMPAREITEM";
		case WM_GETOBJECT:                  return "WM_GETOBJECT";
		case WM_COMPACTING:                 return "WM_COMPACTING";
		case WM_COMMNOTIFY:                 return "WM_COMMNOTIFY";
		case WM_WINDOWPOSCHANGING:          return "WM_WINDOWPOSCHANGING";
		case WM_WINDOWPOSCHANGED:           return "WM_WINDOWPOSCHANGED";
		case WM_POWER:                      return "WM_POWER";
		// case WM_COPYGLOBALDATA:             return "WM_COPYGLOBALDATA";
		case WM_COPYDATA:                   return "WM_COPYDATA";
		case WM_CANCELJOURNAL:              return "WM_CANCELJOURNAL";
		case WM_NOTIFY:                     return "WM_NOTIFY";
		case WM_INPUTLANGCHANGEREQUEST:     return "WM_INPUTLANGCHANGEREQUEST";
		case WM_INPUTLANGCHANGE:            return "WM_INPUTLANGCHANGE";
		case WM_TCARD:                      return "WM_TCARD";
		case WM_HELP:                       return "WM_HELP";
		case WM_USERCHANGED:                return "WM_USERCHANGED";
		case WM_NOTIFYFORMAT:               return "WM_NOTIFYFORMAT";
		case WM_CONTEXTMENU:                return "WM_CONTEXTMENU";
		case WM_STYLECHANGING:              return "WM_STYLECHANGING";
		case WM_STYLECHANGED:               return "WM_STYLECHANGED";
		case WM_DISPLAYCHANGE:              return "WM_DISPLAYCHANGE";
		case WM_GETICON:                    return "WM_GETICON";
		case WM_SETICON:                    return "WM_SETICON";
		case WM_NCCREATE:                   return "WM_NCCREATE";
		case WM_NCDESTROY:                  return "WM_NCDESTROY";
		case WM_NCCALCSIZE:                 return "WM_NCCALCSIZE";
		case WM_NCHITTEST:                  return "WM_NCHITTEST";
		case WM_NCPAINT:                    return "WM_NCPAINT";
		case WM_NCACTIVATE:                 return "WM_NCACTIVATE";
		case WM_GETDLGCODE:                 return "WM_GETDLGCODE";
		case WM_SYNCPAINT:                  return "WM_SYNCPAINT";
		case WM_NCMOUSEMOVE:                return "WM_NCMOUSEMOVE";
		case WM_NCLBUTTONDOWN:              return "WM_NCLBUTTONDOWN";
		case WM_NCLBUTTONUP:                return "WM_NCLBUTTONUP";
		case WM_NCLBUTTONDBLCLK:            return "WM_NCLBUTTONDBLCLK";
		case WM_NCRBUTTONDOWN:              return "WM_NCRBUTTONDOWN";
		case WM_NCRBUTTONUP:                return "WM_NCRBUTTONUP";
		case WM_NCRBUTTONDBLCLK:            return "WM_NCRBUTTONDBLCLK";
		case WM_NCMBUTTONDOWN:              return "WM_NCMBUTTONDOWN";
		case WM_NCMBUTTONUP:                return "WM_NCMBUTTONUP";
		case WM_NCMBUTTONDBLCLK:            return "WM_NCMBUTTONDBLCLK";
		case WM_NCXBUTTONDOWN:              return "WM_NCXBUTTONDOWN";
		case WM_NCXBUTTONUP:                return "WM_NCXBUTTONUP";
		case WM_NCXBUTTONDBLCLK:            return "WM_NCXBUTTONDBLCLK";

		default:
			return "Unknown WM_MESSAGE";
	}
}