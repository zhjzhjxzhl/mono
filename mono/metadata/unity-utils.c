#include <config.h>
#include <mono/utils/mono-publib.h>
#include <mono/metadata/unity-utils.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef WIN32
#include <fcntl.h>
#endif
#include <mono/metadata/object.h>
#include <mono/metadata/metadata.h>
#include <mono/metadata/tabledefs.h>
#include <mono/metadata/class-internals.h>
#include <mono/metadata/object-internals.h>
#include <mono/metadata/marshal.h>
#include <mono/metadata/metadata-internals.h>
#include <mono/metadata/reflection-internals.h>
#include <mono/metadata/threads.h>
#include <mono/metadata/tokentype.h>
#include <mono/metadata/threadpool-ms.h>
#include <mono/utils/mono-string.h>

#include <glib.h>

#ifdef WIN32
#define UTF8_2_WIDE(src,dst) MultiByteToWideChar( CP_UTF8, 0, src, -1, dst, MAX_PATH )
#endif

#undef exit

void unity_mono_exit( int code )
{
	//fprintf( stderr, "mono: exit called, code %d\n", code );
	exit( code );
}


GString* gEmbeddingHostName = 0;


MONO_API void mono_unity_set_embeddinghostname(const char* name)
{
	gEmbeddingHostName = g_string_new(name);
}



MonoString* mono_unity_get_embeddinghostname()
{
	if (gEmbeddingHostName == 0)
		mono_unity_set_embeddinghostname("mono");
	return mono_string_new_wrapper(gEmbeddingHostName->str);
}

static gboolean socket_security_enabled = FALSE;

gboolean
mono_unity_socket_security_enabled_get ()
{
	return socket_security_enabled;
}

void
mono_unity_socket_security_enabled_set (gboolean enabled)
{
	socket_security_enabled = enabled;
}

void mono_unity_set_vprintf_func (vprintf_func func)
{
	//set_vprintf_func (func);
}

MONO_API gboolean
mono_unity_class_is_interface (MonoClass* klass)
{
	return MONO_CLASS_IS_INTERFACE(klass);
}

MONO_API gboolean
mono_unity_class_is_abstract (MonoClass* klass)
{
	return (klass->flags & TYPE_ATTRIBUTE_ABSTRACT);
}

void
unity_mono_install_memory_callbacks(MonoMemoryCallbacks* callbacks)
{
	//g_mem_set_callbacks (callbacks);
}

// classes_ref is a preallocated array of *length_ref MonoClass*
// returned classes are stored in classes_ref, number of stored classes is stored in length_ref
// return value is number of classes found (which may be greater than number of classes stored)
unsigned mono_unity_get_all_classes_with_name_case (MonoImage *image, const char *name, MonoClass **classes_ref, unsigned *length_ref)
{
	MonoClass *klass;
	MonoTableInfo *tdef = &image->tables [MONO_TABLE_TYPEDEF];
	int i, count;
	guint32 attrs, visibility;
	unsigned length = 0;

	/* (yoinked from icall.c) we start the count from 1 because we skip the special type <Module> */
	for (i = 1; i < tdef->rows; ++i)
	{
		klass = mono_class_get (image, (i + 1) | MONO_TOKEN_TYPE_DEF);
		if (klass && klass->name && 0 == mono_utf8_strcasecmp (klass->name, name))
		{
			if (length < *length_ref)
				classes_ref[length] = klass;
			++length;
		}
	}

	if (length < *length_ref)
		*length_ref = length;
	return length;
}

gboolean
unity_mono_method_is_generic (MonoMethod* method)
{
	return method->is_generic;
}

MONO_API MonoMethod*
unity_mono_reflection_method_get_method(MonoReflectionMethod* mrf)
{
	if(!mrf)
		return NULL;

	return mrf->method;
}

MONO_API void
mono_unity_g_free(void *ptr)
{
	g_free (ptr);
}

MONO_API gboolean
mono_class_is_inflated (MonoClass *klass)
{
	g_assert(klass);
	return (klass->is_inflated);
}

MONO_API void
mono_thread_pool_cleanup (void)
{
	mono_threadpool_ms_cleanup ();
}

MONO_API void*
mono_class_get_userdata (MonoClass* klass)
{
	return klass->unity_user_data;
}

MONO_API void
mono_class_set_userdata(MonoClass* klass, void* userdata)
{
	klass->unity_user_data = userdata;
}

MONO_API int
mono_class_get_userdata_offset()
{
	return offsetof(struct _MonoClass, unity_user_data);
}


static UnityFindPluginCallback unity_find_plugin_callback;

MONO_API void
mono_set_find_plugin_callback (UnityFindPluginCallback find)
{
	unity_find_plugin_callback = find;
}

MONO_API UnityFindPluginCallback
mono_get_find_plugin_callback ()
{
	return unity_find_plugin_callback;
}

//object

void mono_unity_object_init(void* obj, MonoClass* klass)
{
	if (klass->valuetype)
		memset(obj, 0, klass->instance_size - sizeof(MonoObject));
	else
		*(MonoObject**)obj = NULL;
}

MonoObject* mono_unity_object_isinst_sealed(MonoObject* obj, MonoClass* targetType)
{
	return obj->vtable->klass == targetType ? obj : NULL;
}

void mono_unity_object_unbox_nullable(MonoObject* obj, MonoClass* nullableArgumentClass, void* storage)
{
	uint32_t valueSize = nullableArgumentClass->instance_size - sizeof(MonoObject);

	if (obj == NULL)
	{
		*((mono_byte*)(storage)+valueSize) = 0;
	}
	else if (obj->vtable->klass != nullableArgumentClass)
	{
		mono_raise_exception(mono_get_exception_invalid_cast());
	}
	else
	{
		memcpy(storage, mono_object_unbox(obj), valueSize);
		*((mono_byte*)(storage)+valueSize) = 1;
	}
}

MonoClass* mono_unity_object_get_class(MonoObject *obj)
{
	return obj->vtable->klass;
}

MonoObject* mono_unity_object_compare_exchange(MonoObject **location, MonoObject *value, MonoObject *comparand)
{
	return ves_icall_System_Threading_Interlocked_CompareExchange_T(location, value, comparand);
}

MonoObject* mono_unity_object_exchange(MonoObject **location, MonoObject *value)
{
	return ves_icall_System_Threading_Interlocked_Exchange_T(location, value);
}

gboolean mono_unity_object_check_box_cast(MonoObject *obj, MonoClass *klass)
{
	return (obj->vtable->klass->element_class == klass->element_class);
}

//class

const char* mono_unity_class_get_image_name(MonoClass* klass)
{
	return klass->image->assembly_name;
}

MonoClass* mono_unity_class_get_generic_definition(MonoClass* klass)
{
	if (klass->generic_class && klass->generic_class->container_class)
		return klass->generic_class->container_class;

	return NULL;
}

MonoClass* mono_unity_class_inflate_generic_class(MonoClass *gklass, MonoGenericContext *context)
{
	MonoError error;
	return mono_class_inflate_generic_class_checked(gklass, context, &error);
}

gboolean mono_unity_class_has_parent_unsafe(MonoClass *klass, MonoClass *parent)
{
	return mono_class_has_parent_fast(klass, parent);
}

MonoAssembly* mono_unity_class_get_assembly(MonoClass *klass)
{
	return klass->image->assembly;
}

gboolean mono_unity_class_is_array(MonoClass *klass)
{
	return klass->rank > 0;
}

MonoClass* mono_unity_class_get_element_class(MonoClass *klass)
{
	return klass->element_class;
}

gboolean mono_unity_class_is_delegate(MonoClass *klass)
{
	return klass->delegate;
}

int mono_unity_class_get_instance_size(MonoClass *klass)
{
	return klass->instance_size;
}

MonoClass* mono_unity_class_get_castclass(MonoClass *klass)
{
	return klass->cast_class;
}

guint32 mono_unity_class_get_native_size(MonoClass* klass)
{
	MonoMarshalType* info = mono_marshal_load_type_info(klass);
	return info->native_size;
}

MonoBoolean mono_unity_class_is_string(MonoClass* klass)
{
	if (mono_class_get_type(klass)->type == MONO_TYPE_STRING)
		return TRUE;
	return FALSE;
}

MonoBoolean mono_unity_class_is_class_type(MonoClass* klass)
{
	if (mono_class_get_type(klass)->type == MONO_TYPE_CLASS)
		return TRUE;
	return FALSE;
}

MONO_API gboolean
mono_class_is_generic(MonoClass *klass)
{
	g_assert(klass);
	return (klass->is_generic);
}

//method

MonoMethod* mono_unity_method_get_generic_definition(MonoMethod* method)
{
	if (method->is_inflated)
		return ((MonoMethodInflated*)method)->declaring;

	return NULL;
}

MonoReflectionMethod* mono_unity_method_get_object(MonoMethod *method)
{
	return mono_method_get_object(mono_domain_get(), method, NULL);
}

MonoMethod* mono_unity_method_alloc0(MonoClass* klass)
{
	return mono_image_alloc0(klass->image, sizeof(MonoMethod));
}

gboolean mono_unity_method_is_static(MonoMethod *method)
{
	return method->flags & METHOD_ATTRIBUTE_STATIC;
}

MonoClass* mono_unity_method_get_class(const MonoMethod *method)
{
	return method->klass;
}

#ifdef IL2CPP_ON_MONO

void* mono_unity_method_get_method_pointer(MonoMethod* method)
{
	return method->method_pointer;
}

void mono_unity_method_set_method_pointer(MonoMethod* method, void *p)
{
	method->method_pointer = p;
}

void* mono_unity_method_get_invoke_pointer(MonoMethod* method)
{
	return method->invoke_pointer;
}

void mono_unity_method_set_invoke_pointer(MonoMethod* method, void *p)
{
	method->invoke_pointer = p;
}

#endif

const char* mono_unity_method_get_name(MonoMethod *method)
{
	return method->name;
}


//must match the hash in il2cpp code generation
static guint32 hash_string_djb2(guchar *str)
{
	guint32 hash = 5381;
	int c;

	while (c = *str++)
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

	return hash;
}

static guint32 get_array_structure_hash(MonoArrayType *atype)
{
	char buffer[100];
	char *ptr = buffer;

	*ptr++ = '[';

	char numbuffer[10];

	for (int i = 0; i < atype->rank; ++i)
	{
		if (atype->numlobounds > 0 && atype->lobounds[i] != 0)
		{
			snprintf(numbuffer, 10, "%d", atype->lobounds[i]);
			char *ptrnum = numbuffer;
			while (*ptrnum)
				*ptr++ = *ptrnum++;

			*ptr++ = ':';
		}

		if (atype->numsizes > 0 && atype->sizes[i] != 0)
		{
			snprintf(numbuffer, 10, "%d", atype->sizes[i]);
			char *ptrnum = numbuffer;
			while (*ptrnum)
				*ptr++ = *ptrnum++;
		}

		if (i < atype->rank - 1)
			*ptr++ = ',';
	}

	*ptr++ = ']';
	*ptr++ = 0;

	return hash_string_djb2(buffer);
}

/* Begin: Hash computation helper functions */

static void get_type_hashes(MonoType *type, GList *hashes);
static void get_type_hashes_generic_inst(MonoGenericInst *inst, GList *hashes);


static void get_type_hashes_generic_inst(MonoGenericInst *inst, GList *hashes)
{
	for (int i = 0; i < inst->type_argc; ++i)
	{
		MonoType *type = inst->type_argv[i];
		get_type_hashes(type, hashes);
	}
}

static void get_type_hashes(MonoType *type, GList *hashes)
{
	if (type->type != MONO_TYPE_GENERICINST)
	{
		MonoClass *klass = NULL;

		switch (type->type)
		{
		case MONO_TYPE_ARRAY:
		{
			MonoArrayType *atype = type->data.array;
			g_list_append(hashes, MONO_TOKEN_TYPE_SPEC);
			g_list_append(hashes, get_array_structure_hash(atype));
			get_type_hashes(&(atype->eklass->this_arg), hashes);
			break;
		}
		case MONO_TYPE_CLASS:
		case MONO_TYPE_VALUETYPE:
			klass = type->data.klass;
			break;
		case MONO_TYPE_BOOLEAN:
			klass = mono_defaults.boolean_class;
			break;
		case MONO_TYPE_CHAR:
			klass = mono_defaults.char_class;
			break;
		case MONO_TYPE_I:
			klass = mono_defaults.int_class;
			break;
		case MONO_TYPE_U:
			klass = mono_defaults.uint_class;
			break;
		case MONO_TYPE_I1:
			klass = mono_defaults.sbyte_class;
			break;
		case MONO_TYPE_U1:
			klass = mono_defaults.byte_class;
			break;
		case MONO_TYPE_I2:
			klass = mono_defaults.int16_class;
			break;
		case MONO_TYPE_U2:
			klass = mono_defaults.uint16_class;
			break;
		case MONO_TYPE_I4:
			klass = mono_defaults.int32_class;
			break;
		case MONO_TYPE_U4:
			klass = mono_defaults.uint32_class;
			break;
		case MONO_TYPE_I8:
			klass = mono_defaults.int64_class;
			break;
		case MONO_TYPE_U8:
			klass = mono_defaults.uint64_class;
			break;
		case MONO_TYPE_R4:
			klass = mono_defaults.single_class;
			break;
		case MONO_TYPE_R8:
			klass = mono_defaults.double_class;
			break;
		case MONO_TYPE_STRING:
			klass = mono_defaults.string_class;
			break;
		case MONO_TYPE_OBJECT:
			klass = mono_defaults.object_class;
			break;
		}

		if (klass)
		{
			g_list_append(hashes, klass->type_token);
			g_list_append(hashes, hash_string_djb2(klass->image->module_name));
		}

		return;
	}
	else
	{
		g_list_append(hashes, type->data.generic_class->container_class->type_token);
		g_list_append(hashes, hash_string_djb2(type->data.generic_class->container_class->image->module_name));
		get_type_hashes_generic_inst(type->data.generic_class->context.class_inst, hashes);
	}

}

static GList* get_type_hashes_method(MonoMethod *method)
{
	GList *hashes = monoeg_g_list_alloc();

	hashes->data = method->token;
	g_list_append(hashes, hash_string_djb2(method->klass->image->module_name));

	if (method->klass->is_inflated)
	{
		g_list_append(hashes, method->klass->type_token);
		get_type_hashes_generic_inst(method->klass->generic_class->context.class_inst, hashes);
	}

	if (method->is_inflated)
	{
		MonoGenericContext* methodGenericContext = mono_method_get_context(method);
		if (methodGenericContext->method_inst != NULL)
			get_type_hashes_generic_inst(methodGenericContext->method_inst, hashes);
	}

	return hashes;
}

//hash combination function must match the one used in IL2CPP codegen
static guint64 combine_hashes(guint64 hash1, guint64 hash2)
{
	const guint64 seed = 486187739;
	return hash1 * seed + hash2;
}

static void combine_all_hashes(gpointer data, gpointer user_data)
{
	guint64 *hash = (guint64*)user_data;
	if (*hash == 0)
		*hash = (guint64)data;
	else
		*hash = combine_hashes(*hash, (guint64)(uintptr_t)data);
}

/* End: Hash computation helper functions */

guint64 mono_unity_method_get_hash(MonoMethod *method)
{
	GList *hashes = get_type_hashes_method(method);

	guint64 hash = 0;

	g_list_first(hashes);
	g_list_foreach(hashes, combine_all_hashes, &hash);
	g_list_free(hashes);

	return hash;
}

MonoMethod* mono_unity_method_get_aot_array_helper_from_wrapper(MonoMethod *method)
{
	MonoMethod *m;
	const char *prefix;
	MonoGenericContext ctx;
	MonoType *args[16];
	char *mname, *iname, *s, *s2, *helper_name = NULL;

	prefix = "System.Collections.Generic";
	s = g_strdup_printf("%s", method->name + strlen(prefix) + 1);
	s2 = strstr(s, "`1.");
	g_assert(s2);
	s2[0] = '\0';
	iname = s;
	mname = s2 + 3;

	//printf ("X: %s %s\n", iname, mname);

	if (!strcmp(iname, "IList"))
		helper_name = g_strdup_printf("InternalArray__%s", mname);
	else
		helper_name = g_strdup_printf("InternalArray__%s_%s", iname, mname);
	m = mono_class_get_method_from_name(mono_defaults.array_class, helper_name, mono_method_signature(method)->param_count);
	g_assert(m);
	g_free(helper_name);
	g_free(s);

	if (m->is_generic) {
		MonoError error;
		memset(&ctx, 0, sizeof(ctx));
		args[0] = &method->klass->element_class->byval_arg;
		ctx.method_inst = mono_metadata_get_generic_inst(1, args);
		m = mono_class_inflate_generic_method_checked(m, &ctx, &error);
		g_assert(mono_error_ok(&error)); /* FIXME don't swallow the error */
	}

	return m;
}

MonoObject* mono_unity_method_convert_return_type_if_needed(MonoMethod *method, void *value)
{
	if (method->signature && method->signature->ret->type == MONO_TYPE_PTR)
	{
		MonoError unused;
		return mono_value_box_checked(mono_domain_get(), mono_defaults.int_class, &value, &unused);
	}

	return (MonoObject*)value;
}

MONO_API gboolean
unity_mono_method_is_inflated(MonoMethod* method)
{
	return method->is_inflated;
}

guint32 mono_unity_method_get_token(MonoMethod *method)
{
	return method->token;
}

//domain


void mono_unity_domain_install_finalize_runtime_invoke(MonoDomain* domain, RuntimeInvokeFunction callback)
{
	domain->finalize_runtime_invoke = callback;
}

void mono_unity_domain_install_capture_context_runtime_invoke(MonoDomain* domain, RuntimeInvokeFunction callback)
{
	domain->capture_context_runtime_invoke = callback;
}

void mono_unity_domain_install_capture_context_method(MonoDomain* domain, gpointer callback)
{
	domain->capture_context_method = callback;
}

//array

int mono_unity_array_get_element_size(MonoArray *arr)
{
	return arr->obj.vtable->klass->sizes.element_size;
}

MonoClass* mono_unity_array_get_class(MonoArray *arr)
{
	return arr->obj.vtable->klass;
}

mono_array_size_t mono_unity_array_get_max_length(MonoArray *arr)
{
	return arr->max_length;
}

//type

gboolean mono_unity_type_is_generic_instance(MonoType *type)
{
	return type->type == MONO_TYPE_GENERICINST;
}

MonoGenericClass* mono_unity_type_get_generic_class(MonoType *type)
{
	if (type->type != MONO_TYPE_GENERICINST)
		return NULL;

	return type->data.generic_class;
}

gboolean mono_unity_type_is_enum_type(MonoType *type)
{
	if (type->type == MONO_TYPE_VALUETYPE && type->data.klass->enumtype)
		return TRUE;
	if (type->type == MONO_TYPE_GENERICINST && type->data.generic_class->container_class->enumtype)
		return TRUE;
	return FALSE;
}

gboolean mono_unity_type_is_boolean(MonoType *type)
{
	return type->type == MONO_TYPE_BOOLEAN;
}

MonoClass* mono_unity_type_get_element_class(MonoType *type)
{
	return type->data.klass->element_class;
}

//generic class

MonoGenericContext mono_unity_generic_class_get_context(MonoGenericClass *klass)
{
	return klass->context;
}

MonoClass* mono_unity_generic_class_get_container_class(MonoGenericClass *klass)
{
	return klass->container_class;
}

//method signature

MonoClass* mono_unity_signature_get_class_for_param(MonoMethodSignature *sig, int index)
{
	MonoType *type = sig->params[index];
	return mono_class_from_mono_type(type);
}

int mono_unity_signature_num_parameters(MonoMethodSignature *sig)
{
	return sig->param_count;
}

gboolean mono_unity_signature_param_is_byref(MonoMethodSignature *sig, int index)
{
	return sig->params[index]->byref;
}

//generic inst

guint mono_unity_generic_inst_get_type_argc(MonoGenericInst *inst)
{
	return inst->type_argc;
}

MonoType* mono_unity_generic_inst_get_type_argument(MonoGenericInst *inst, int index)
{
	return inst->type_argv[index];
}

//exception

MonoString* mono_unity_exception_get_message(MonoException *exc)
{
	return exc->message;
}

MonoString* mono_unity_exception_get_stack_trace(MonoException *exc)
{
	return exc->stack_trace;
}

MonoObject* mono_unity_exception_get_inner_exception(MonoException *exc)
{
	return exc->inner_ex;
}

MonoArray* mono_unity_exception_get_trace_ips(MonoException *exc)
{
	return exc->trace_ips;
}

void mono_unity_exception_set_trace_ips(MonoException *exc, MonoArray *ips)
{
	assert(sizeof((exc)->trace_ips == sizeof(void**)));
	mono_gc_wbarrier_set_field((MonoObject*)(exc), &((exc)->trace_ips), (MonoObject*)ips);
}

MonoException* mono_unity_exception_get_marshal_directive(const char* msg)
{
	return mono_exception_from_name_msg(mono_get_corlib(), "System.Runtime.InteropServices", "MarshalDirectiveException", msg);
}

//defaults

MonoClass* mono_unity_defaults_get_int_class()
{
	return mono_defaults.int_class;
}

MonoClass* mono_unity_defaults_get_stack_frame_class()
{
	return mono_defaults.stack_frame_class;
}

MonoClass* mono_unity_defaults_get_int32_class()
{
	return mono_defaults.int32_class;
}

MonoClass* mono_unity_defaults_get_char_class()
{
	return mono_defaults.char_class;
}

MonoClass* mono_unity_defaults_get_delegate_class()
{
	return mono_defaults.delegate_class;
}

MonoClass* mono_unity_defaults_get_byte_class()
{
	return mono_defaults.byte_class;
}

//misc

MonoAssembly* mono_unity_assembly_get_mscorlib()
{
	return mono_defaults.corlib->assembly;
}

MonoImage* mono_unity_image_get_mscorlib()
{
	return mono_defaults.corlib->assembly->image;
}

MonoClass* mono_unity_generic_container_get_parameter_class(MonoGenericContainer* generic_container, gint index)
{
	MonoGenericParam *param = mono_generic_container_get_param(generic_container, index);
	return mono_class_from_generic_parameter_internal(param);
}

MonoString* mono_unity_string_append_assembly_name_if_necessary(MonoString* typeName, const char* assemblyName)
{
	if (typeName != NULL)
	{
		MonoTypeNameParse info;

		// The mono_reflection_parse_type function will mangle the name, so don't use this copy later.
		MonoError unused;
		char* nameForParsing = mono_string_to_utf8_checked(typeName, &unused);
		if (mono_reflection_parse_type(nameForParsing, &info))
		{
			if (!info.assembly.name)
			{
				GString* assemblyQualifiedName = g_string_new(0);
				char* name = mono_string_to_utf8_checked(typeName, &unused);
				g_string_append_printf(assemblyQualifiedName, "%s, %s", name, assemblyName);

				typeName = mono_string_new(mono_domain_get(), assemblyQualifiedName->str);

				g_string_free(assemblyQualifiedName, FALSE);
				mono_free(name);
			}
		}

		mono_free(nameForParsing);
	}

	return typeName;
}

void mono_unity_memory_barrier()
{
	mono_memory_barrier();
}

MonoObject* mono_unity_delegate_get_target(MonoDelegate *delegate)
{
	return delegate->target;
}

gchar* mono_unity_get_runtime_build_info(const char *date, const char *time)
{
	return g_strdup_printf("Unity IL2CPP(%s %s)", date, time);
}

void* mono_unity_get_field_address(MonoObject *obj, MonoVTable *vt, MonoClassField *field)
{
	// This is a copy of mono_field_get_addr - we need to consider how to expose that on the public API.
	MONO_REQ_GC_UNSAFE_MODE;

	guint8 *src;

	if (field->type->attrs & FIELD_ATTRIBUTE_STATIC) {
		if (field->offset == -1) {
			/* Special static */
			gpointer addr;

			mono_domain_lock(vt->domain);
			addr = g_hash_table_lookup(vt->domain->special_static_fields, field);
			mono_domain_unlock(vt->domain);
			src = (guint8 *)mono_get_special_static_data(GPOINTER_TO_UINT(addr));
		}
		else {
			src = (guint8*)mono_vtable_get_static_field_data(vt) + field->offset;
		}
	}
	else {
		src = (guint8*)obj + field->offset;
	}

	return src;
}

gboolean mono_unity_thread_state_init_from_handle(MonoThreadUnwindState *tctx, MonoThreadInfo *info)
{
	tctx->valid = TRUE;
	tctx->unwind_data[MONO_UNWIND_DATA_DOMAIN] = mono_domain_get();
	tctx->unwind_data[MONO_UNWIND_DATA_LMF] = NULL;
	tctx->unwind_data[MONO_UNWIND_DATA_JIT_TLS] = NULL;

	return TRUE;
}

void mono_unity_stackframe_set_method(MonoStackFrame *sf, MonoMethod *method)
{
	assert(sizeof(sf->method) == sizeof(void**));
	mono_gc_wbarrier_set_field((MonoObject*)(sf), &(sf->method), (MonoObject*)mono_method_get_object(mono_domain_get(), method, NULL));
}

MonoType* mono_unity_reflection_type_get_type(MonoReflectionType *type)
{
	return type->type;
}
