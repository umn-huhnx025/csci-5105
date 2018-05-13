/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#include "pubsub.h"

bool_t
xdr_joinserver_1_argument (XDR *xdrs, joinserver_1_argument *objp)
{
	 if (!xdr_string (xdrs, &objp->IP, ~0))
		 return FALSE;
	 if (!xdr_int (xdrs, &objp->ProgID))
		 return FALSE;
	 if (!xdr_int (xdrs, &objp->ProgVers))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_leaveserver_1_argument (XDR *xdrs, leaveserver_1_argument *objp)
{
	 if (!xdr_string (xdrs, &objp->IP, ~0))
		 return FALSE;
	 if (!xdr_int (xdrs, &objp->ProgID))
		 return FALSE;
	 if (!xdr_int (xdrs, &objp->ProgVers))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_join_1_argument (XDR *xdrs, join_1_argument *objp)
{
	 if (!xdr_string (xdrs, &objp->IP, ~0))
		 return FALSE;
	 if (!xdr_int (xdrs, &objp->Port))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_leave_1_argument (XDR *xdrs, leave_1_argument *objp)
{
	 if (!xdr_string (xdrs, &objp->IP, ~0))
		 return FALSE;
	 if (!xdr_int (xdrs, &objp->Port))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_subscribe_1_argument (XDR *xdrs, subscribe_1_argument *objp)
{
	 if (!xdr_string (xdrs, &objp->IP, ~0))
		 return FALSE;
	 if (!xdr_int (xdrs, &objp->Port))
		 return FALSE;
	 if (!xdr_string (xdrs, &objp->Article, ~0))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_unsubscribe_1_argument (XDR *xdrs, unsubscribe_1_argument *objp)
{
	 if (!xdr_string (xdrs, &objp->IP, ~0))
		 return FALSE;
	 if (!xdr_int (xdrs, &objp->Port))
		 return FALSE;
	 if (!xdr_string (xdrs, &objp->Article, ~0))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_publish_1_argument (XDR *xdrs, publish_1_argument *objp)
{
	 if (!xdr_string (xdrs, &objp->Article, ~0))
		 return FALSE;
	 if (!xdr_string (xdrs, &objp->IP, ~0))
		 return FALSE;
	 if (!xdr_int (xdrs, &objp->Port))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_publishserver_1_argument (XDR *xdrs, publishserver_1_argument *objp)
{
	 if (!xdr_string (xdrs, &objp->Article, ~0))
		 return FALSE;
	 if (!xdr_string (xdrs, &objp->IP, ~0))
		 return FALSE;
	 if (!xdr_int (xdrs, &objp->Port))
		 return FALSE;
	return TRUE;
}