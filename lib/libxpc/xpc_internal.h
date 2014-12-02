#include <nv.h>

#define _XPC_TYPE_INVALID		0
#define _XPC_TYPE_DICTIONARY	1
#define _XPC_TYPE_ARRAY			2
#define _XPC_TYPE_BOOL			3
#define _XPC_TYPE_CONNECTION	4
#define _XPC_TYPE_ENDPOINT		5

#define _XPC_TYPE_INT64			8
#define _XPC_TYPE_UINT64		9
#define _XPC_TYPE_DATE			10
#define _XPC_TYPE_DATA			11
#define _XPC_TYPE_STRING		12
#define _XPC_TYPE_UUID			13
#define _XPC_TYPE_FD			14
#define _XPC_TYPE_SHMEM			15
#define _XPC_TYPE_ERROR			16
#define _XPC_TYPE_DOUBLE		17
#define _XPC_TYPE_MAX			_XPC_TYPE_DOUBLE

typedef union {
	nvlist_t *nv;
	uint64_t ui;
	int64_t i;
	const char *str;
	bool b;
	uintptr_t ptr;
} xpc_u;	


#define _XPC_FROM_WIRE 0x1
struct xpc_object {
	uint8_t xo_xpc_type;
	uint16_t xo_flags;
	volatile uint32_t xo_refcnt;
	size_t xo_size;
	xpc_u xo_u;
	audit_token_t *xo_audit_token;
};
#define xo_nv xo_u.nv
#define xo_str xo_u.str
#define xo_bool xo_u.b
#define xo_uint xo_u.ui
#define xo_int xo_u.i
#define xo_ptr xo_u.ptr
__private_extern__ struct xpc_object *_xpc_prim_create(int type, xpc_u value, size_t size);
__private_extern__ struct xpc_object *_xpc_prim_create_flags(int type, xpc_u value, size_t size, uint16_t flags);
__private_extern__ nvlist_t *xpc2nv(xpc_object_t obj);
__private_extern__ void nv_release_entry(nvlist_t *nv, const char *key);
__private_extern__ int nvlist_exists_object(const nvlist_t *nv, const char *key);
__private_extern__ void nvlist_add_object(nvlist_t *nv, const char *key, xpc_object_t xobj);
__private_extern__ xpc_object_t nvlist_get_object(const nvlist_t *nv, const char *key);
__private_extern__ xpc_object_t nvlist_move_object(const nvlist_t *nv, const char *key);
__private_extern__ size_t nvcount(const nvlist_t *nv);
