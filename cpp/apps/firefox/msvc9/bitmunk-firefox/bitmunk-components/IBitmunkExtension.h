/*
 * DO NOT EDIT.  THIS FILE IS GENERATED FROM cpp/apps/firefox/components/IBitmunkExtension.idl
 */

#ifndef __gen_IBitmunkExtension_h__
#define __gen_IBitmunkExtension_h__


#ifndef __gen_nsISupports_h__
#include "nsISupports.h"
#endif

/* For IDL files that don't want to include root IDL files. */
#ifndef NS_NO_VTABLE
#define NS_NO_VTABLE
#endif

/* starting interface:    nsIBitmunkExtension */
#define NS_IBITMUNKEXTENSION_IID_STR "f1dab993-22bb-4416-9471-afd19dc9dd85"

#define NS_IBITMUNKEXTENSION_IID \
  {0xf1dab993, 0x22bb, 0x4416, \
    { 0x94, 0x71, 0xaf, 0xd1, 0x9d, 0xc9, 0xdd, 0x85 }}

class NS_NO_VTABLE NS_SCRIPTABLE nsIBitmunkExtension : public nsISupports {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IBITMUNKEXTENSION_IID)

  /* boolean getBitmunkPort (out long port); */
  NS_SCRIPTABLE NS_IMETHOD GetBitmunkPort(PRInt32 *port, PRBool *_retval) = 0;

  /* boolean launchBitmunk (out long pid); */
  NS_SCRIPTABLE NS_IMETHOD LaunchBitmunk(PRInt32 *pid, PRBool *_retval) = 0;

  /* boolean terminateBitmunk (in long pid); */
  NS_SCRIPTABLE NS_IMETHOD TerminateBitmunk(PRInt32 pid, PRBool *_retval) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(nsIBitmunkExtension, NS_IBITMUNKEXTENSION_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_NSIBITMUNKEXTENSION \
  NS_SCRIPTABLE NS_IMETHOD GetBitmunkPort(PRInt32 *port, PRBool *_retval); \
  NS_SCRIPTABLE NS_IMETHOD LaunchBitmunk(PRInt32 *pid, PRBool *_retval); \
  NS_SCRIPTABLE NS_IMETHOD TerminateBitmunk(PRInt32 pid, PRBool *_retval); 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_NSIBITMUNKEXTENSION(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetBitmunkPort(PRInt32 *port, PRBool *_retval) { return _to GetBitmunkPort(port, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD LaunchBitmunk(PRInt32 *pid, PRBool *_retval) { return _to LaunchBitmunk(pid, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD TerminateBitmunk(PRInt32 pid, PRBool *_retval) { return _to TerminateBitmunk(pid, _retval); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_NSIBITMUNKEXTENSION(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetBitmunkPort(PRInt32 *port, PRBool *_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetBitmunkPort(port, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD LaunchBitmunk(PRInt32 *pid, PRBool *_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->LaunchBitmunk(pid, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD TerminateBitmunk(PRInt32 pid, PRBool *_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->TerminateBitmunk(pid, _retval); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class nsBitmunkExtension : public nsIBitmunkExtension
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIBITMUNKEXTENSION

  nsBitmunkExtension();

private:
  ~nsBitmunkExtension();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(nsBitmunkExtension, nsIBitmunkExtension)

nsBitmunkExtension::nsBitmunkExtension()
{
  /* member initializers and constructor code */
}

nsBitmunkExtension::~nsBitmunkExtension()
{
  /* destructor code */
}

/* boolean getBitmunkPort (out long port); */
NS_IMETHODIMP nsBitmunkExtension::GetBitmunkPort(PRInt32 *port, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* boolean launchBitmunk (out long pid); */
NS_IMETHODIMP nsBitmunkExtension::LaunchBitmunk(PRInt32 *pid, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* boolean terminateBitmunk (in long pid); */
NS_IMETHODIMP nsBitmunkExtension::TerminateBitmunk(PRInt32 pid, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


#endif /* __gen_IBitmunkExtension_h__ */
