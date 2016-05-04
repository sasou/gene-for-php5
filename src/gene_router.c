/*
  +----------------------------------------------------------------------+
  | gene                                                                 |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Sasou  <admin@php-gene.com> web:www.php-gene.com             |
  +----------------------------------------------------------------------+
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "main/SAPI.h"
#include "Zend/zend_API.h"
#include "Zend/zend_exceptions.h"
#include "Zend/zend_alloc.h"
#include "ext/reflection/php_reflection.h"
#include "ext/spl/spl_directory.h"



#include "php_gene.h"
#include "gene_router.h"
#include "gene_cache.h"
#include "gene_common.h"

zend_class_entry *gene_router_ce;

/** {{{ ARG_INFO
 */
ZEND_BEGIN_ARG_INFO_EX(gene_router_call_arginfo, 0, 0, 2)
	ZEND_ARG_INFO(0, name)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()
/* }}} */

/** {{{ static void get_path_router(char *keyString, int keyString_len TSRMLS_DC)
 */
zval ** get_path_router(zval **val,const char *path,zval *param TSRMLS_DC)
{
	zval **ret = NULL,**tmp = NULL,**leaf = NULL,*var;
	char *paths,*seg = NULL,*ptr = NULL,*key = NULL;
	int keylen;
	long idx;
	paths = estrndup(path,strlen(path));
	//php_printf(" paths:%s " , paths);
	if (strlen(paths)==0) {
		if (zend_hash_find((*val)->value.ht, "leaf", 5, (void **)&leaf) == FAILURE){
			leaf = NULL;
		}
	} else {
		seg = php_strtok_r(paths, "/", &ptr);
		if (ptr && strlen(seg)>0) {
			if (zend_hash_find((*val)->value.ht,seg, strlen(seg)+1, (void **)&ret) == SUCCESS){
				leaf =  get_path_router(ret,ptr,param TSRMLS_CC);
			} else {
				if (zend_hash_find((*val)->value.ht, "chird", 6, (void **)&ret) == SUCCESS){
					for(zend_hash_internal_pointer_reset((*ret)->value.ht);zend_hash_has_more_elements((*ret)->value.ht) == SUCCESS;zend_hash_move_forward((*ret)->value.ht)) {
						if (zend_hash_get_current_key_ex((*ret)->value.ht, &key, &keylen, &idx, 0, NULL) == HASH_KEY_IS_LONG) {
							if (zend_hash_get_current_data((*ret)->value.ht, (void**)&tmp) == SUCCESS) {
								leaf = get_path_router(tmp,ptr,param TSRMLS_CC);
								if (leaf ) {
									MAKE_STD_ZVAL(var);
									ZVAL_STRING(var,seg,1);
									zend_hash_next_index_insert(param->value.ht,&var, sizeof(zval **), NULL);
									break;
								}
							}

						} else {
							if (zend_hash_get_current_data((*ret)->value.ht, (void**)&tmp) == SUCCESS) {
								leaf = get_path_router(tmp,ptr,param TSRMLS_CC);
								if (leaf ) {
									MAKE_STD_ZVAL(var);
									ZVAL_STRING(var,seg,1);
									zend_hash_update(param->value.ht,key, keylen+1, &var, sizeof(zval **), NULL);
									break;
								}
							}
						}
					}
				}
			}
		} else {
			if (zend_hash_find((*val)->value.ht,seg, strlen(seg)+1, (void **)&ret) == SUCCESS){
				if (zend_hash_find((*ret)->value.ht, "leaf", 5, (void **)&leaf) == FAILURE){
					leaf = NULL;
				}
			} else {
				if (zend_hash_find((*val)->value.ht, "chird", 6, (void **)&ret) == SUCCESS){
					for(zend_hash_internal_pointer_reset((*ret)->value.ht);zend_hash_has_more_elements((*ret)->value.ht) == SUCCESS;zend_hash_move_forward((*ret)->value.ht)) {
						if (zend_hash_get_current_key_ex((*ret)->value.ht, &key, &keylen, &idx, 0, NULL) == HASH_KEY_IS_LONG) {
							if (zend_hash_get_current_data((*ret)->value.ht, (void**)&tmp) == SUCCESS) {
								if (zend_hash_find((*tmp)->value.ht, "leaf", 5, (void **)&leaf) == SUCCESS){
									MAKE_STD_ZVAL(var);
									ZVAL_STRING(var,seg,1);
									zend_hash_index_update(param->value.ht,idx, &var, sizeof(zval **), NULL);
									break;
								}
							}

						} else {
							if (zend_hash_get_current_data((*ret)->value.ht, (void**)&tmp) == SUCCESS) {
								if (zend_hash_find((*tmp)->value.ht, "leaf", 5, (void **)&leaf) == SUCCESS){
									MAKE_STD_ZVAL(var);
									ZVAL_STRING(var,seg,1);
									zend_hash_update(param->value.ht,key, keylen+1, &var, sizeof(zval **), NULL);
									break;
								}
							}
						}
					}
				}
			}
		}
	}
	efree(paths);
	return leaf;
}
/* }}} */


/** {{{ static void get_router_info(char *keyString, int keyString_len TSRMLS_DC)
 */
int get_router_info(zval **leaf,zval **cacheHook TSRMLS_DC)
{
	zval **hname,**before,**after,**m,**h;
	int size = 0,is = 1;
	char *run = NULL,*hookname = NULL,*seg = NULL,*ptr = NULL;

	if (zend_hash_find((*leaf)->value.ht, "hook", 5, (void **)&hname) == SUCCESS){
		spprintf(&hookname, 0, "hook:%s", Z_STRVAL_PP(hname));
	}
	if (hookname) {
		seg = php_strtok_r(hookname, "@", &ptr);
	}

	size = 1;
	run = (char *) ecalloc(size,sizeof(char));
	if (ptr && strlen(ptr)>0) {
		if ((strcmp(ptr, "clearBefore") == 0) || (strcmp(ptr, "clearAll") == 0)) {
			is = 0;
		}
	}
	if (is == 1 && zend_hash_find((*cacheHook)->value.ht, "hook:before", 12, (void **)&before) == SUCCESS){
		size = size+Z_STRLEN_PP(before);
		run = erealloc(run,size);
		strcat(run,Z_STRVAL_PP(before));
		run[size-1] = 0;
	}
	is = 1;
	if (seg) {
		if (zend_hash_find((*cacheHook)->value.ht, seg, strlen(seg)+1, (void **)&h) == SUCCESS){
			efree(hookname);
			hookname = NULL;
			size = size+Z_STRLEN_PP(h);
			run = erealloc(run,size);
			strcat(run,Z_STRVAL_PP(h));
			run[size-1] = 0;
		}
	}
	if (zend_hash_find((*leaf)->value.ht, "run", 4, (void **)&m) == SUCCESS){
		size = size+Z_STRLEN_PP(m);
		run = erealloc(run,size);
		strcat(run,Z_STRVAL_PP(m));
		run[size-1] = 0;
	}
	if (ptr && strlen(ptr)>0) {
		if ((strcmp(ptr, "clearAfter") == 0) || (strcmp(ptr, "clearAll") == 0)) {
			is = 0;
		}
	}
	if (is == 1 && zend_hash_find((*cacheHook)->value.ht, "hook:after", 11, (void **)&after) == SUCCESS){
		size = size+Z_STRLEN_PP(after);
		run = erealloc(run,size);
		strcat(run,Z_STRVAL_PP(after));
		run[size-1] = 0;
	}
	zend_try {
		//php_printf(" run:%p; ",run);
		zend_eval_stringl(run, strlen(run), NULL, "" TSRMLS_CC);
	} zend_catch {
		efree(run);
		run = NULL;
		if (hookname) {
			efree(hookname);
			hookname = NULL;
		}
		zend_bailout();
	} zend_end_try();
	efree(run);
	if (hookname) {
		efree(hookname);
		hookname = NULL;
	}
	run = NULL;
	return 1;
}
/* }}} */

zval * request_query(int type, char * name, int len TSRMLS_DC) {
	zval 	**carrier, **ret;


	switch (type) {
		case TRACK_VARS_POST:
		case TRACK_VARS_GET:
		case TRACK_VARS_FILES:
		case TRACK_VARS_COOKIE:
			carrier = &PG(http_globals)[type];
			break;
		case TRACK_VARS_ENV:
			carrier = &PG(http_globals)[type];
			break;
		case TRACK_VARS_SERVER:
			carrier = &PG(http_globals)[type];
			break;
		case TRACK_VARS_REQUEST:
			(void)zend_hash_find(&EG(symbol_table), ZEND_STRS("_REQUEST"), (void **)&carrier);
			break;
		default:
			break;
	}

	if (!carrier || !(*carrier)) {
		zval *empty;
		MAKE_STD_ZVAL(empty);
		ZVAL_NULL(empty);
		return empty;
	}

	if (!len) {
		Z_ADDREF_P(*carrier);
		return *carrier;
	}

	if (zend_hash_find(Z_ARRVAL_PP(carrier), name, len + 1, (void **)&ret) == FAILURE) {
		zval *empty;
		MAKE_STD_ZVAL(empty);
		ZVAL_NULL(empty);
		return empty;
	}

	Z_ADDREF_P(*ret);
	return *ret;
}


/** {{{ static void get_function_content(char *keyString, int keyString_len TSRMLS_DC)
 */
char * get_function_content(zval **content TSRMLS_DC)
{
	zval *objEx,*ret,*fileName,*arg,*arg1;
	int startline,endline;
	zval *params[3];
	char *result=NULL,*tmp = NULL;

	MAKE_STD_ZVAL(objEx);
	MAKE_STD_ZVAL(ret);
	MAKE_STD_ZVAL(arg);
	object_init_ex(objEx, reflection_function_ptr ZEND_FILE_LINE_CC TSRMLS_CC);
	ZVAL_STRING(arg,"__construct",1);
	params[0] = *content;
	call_user_function(NULL, &objEx, arg, ret,1,params TSRMLS_CC);
	zval_ptr_dtor(&ret);
	zval_ptr_dtor(&arg);

	MAKE_STD_ZVAL(arg);
	MAKE_STD_ZVAL(fileName);
	ZVAL_STRING(arg,"getFileName",1);
	call_user_function(NULL, &objEx, arg, fileName,0,NULL TSRMLS_CC);
	zval_ptr_dtor(&arg);

	MAKE_STD_ZVAL(ret);
	MAKE_STD_ZVAL(arg);
	ZVAL_STRING(arg,"getStartLine",1);
	call_user_function(NULL, &objEx, arg, ret,0,NULL TSRMLS_CC);
	startline = Z_LVAL_P(ret)-1;
	zval_ptr_dtor(&ret);
	zval_ptr_dtor(&arg);

	MAKE_STD_ZVAL(ret);
	MAKE_STD_ZVAL(arg);
	ZVAL_STRING(arg,"getEndLine",1);
	call_user_function(NULL, &objEx, arg, ret,0,NULL TSRMLS_CC);
	endline = Z_LVAL_P(ret);
	zval_ptr_dtor(&ret);
	zval_ptr_dtor(&arg);
	zval_ptr_dtor(&objEx);

	//get codestartline
	MAKE_STD_ZVAL(objEx);
	MAKE_STD_ZVAL(ret);
	MAKE_STD_ZVAL(arg);
	object_init_ex(objEx, spl_ce_SplFileObject ZEND_FILE_LINE_CC TSRMLS_CC);
	ZVAL_STRING(arg,"__construct",1);
	params[0] = fileName;
	call_user_function(NULL, &objEx, arg, ret,1,params TSRMLS_CC);
	zval_ptr_dtor(&ret);
	zval_ptr_dtor(&arg);
	zval_ptr_dtor(&fileName);

	MAKE_STD_ZVAL(ret);
	MAKE_STD_ZVAL(arg);
	MAKE_STD_ZVAL(fileName);
	ZVAL_STRING(arg,"seek",1);
	ZVAL_LONG(fileName,startline);
	params[0] = fileName;
	call_user_function(NULL, &objEx, arg, ret,1,params TSRMLS_CC);
	zval_ptr_dtor(&ret);
	zval_ptr_dtor(&arg);
	zval_ptr_dtor(&fileName);

    while (startline < endline){
    	MAKE_STD_ZVAL(ret);
		MAKE_STD_ZVAL(arg);
		ZVAL_STRING(arg,"current",1);
		call_user_function(NULL, &objEx, arg, ret,0,NULL TSRMLS_CC);
		spprintf(&tmp, 0, "%s", Z_STRVAL_P(ret));
		zval_ptr_dtor(&ret);
		zval_ptr_dtor(&arg);

		if (result) {
			result = erealloc(result,strlen(result)+strlen(tmp)+1);
			strcat(result,tmp);
		} else {
			result = ecalloc(strlen(tmp)+1,sizeof(char));
			strcpy (result,tmp);
		}
		efree(tmp);
		++startline;

		MAKE_STD_ZVAL(ret);
		MAKE_STD_ZVAL(arg);
		ZVAL_STRING(arg,"next",1);
		call_user_function(NULL, &objEx, arg, ret,0,NULL TSRMLS_CC);
		zval_ptr_dtor(&ret);
		zval_ptr_dtor(&arg);
    }
    zval_ptr_dtor(&objEx);

	MAKE_STD_ZVAL(ret);
	MAKE_STD_ZVAL(arg);
    ZVAL_STRING(arg,"strpos",1);
	MAKE_STD_ZVAL(fileName);
	ZVAL_STRING(fileName,"function",1);
	MAKE_STD_ZVAL(arg1);
	ZVAL_STRING(arg1,result,1);
	params[0] = arg1;
	params[1] = fileName;
	call_user_function(NULL, NULL, arg, ret,2,params TSRMLS_CC);
	startline = Z_LVAL_P(ret);
	zval_ptr_dtor(&ret);
	zval_ptr_dtor(&fileName);
	zval_ptr_dtor(&arg);


	MAKE_STD_ZVAL(ret);
	MAKE_STD_ZVAL(arg);
	ZVAL_STRING(arg,"strrpos",1);
	MAKE_STD_ZVAL(fileName);
	ZVAL_STRING(fileName,"}",1);
	params[1] = fileName;
	call_user_function(NULL, NULL, arg, ret,2,params TSRMLS_CC);
	endline = Z_LVAL_P(ret);
	zval_ptr_dtor(&ret);
	zval_ptr_dtor(&fileName);
	zval_ptr_dtor(&arg);
	zval_ptr_dtor(&arg1);

	tmp = emalloc(endline-startline+1);
	mid(tmp,result, endline-startline+1,startline);
	efree(result);
	remove_extra_space(tmp);
	params[0] = NULL;
	params[1] = NULL;
	params[2] = NULL;
	return tmp;
}
/* }}} */



/** {{{ char get_router_content(char *content TSRMLS_DC)
 */
char * get_router_content_F(char *src,char *method,char *path TSRMLS_DC)
{
	char *dist;
	if (strcmp(method, "hook") == 0) {
		if (strcmp(path, "before") == 0) {
			spprintf(&dist, 0, GENE_ROUTER_CONTENT_FB, src);
		} else if (strcmp(path, "after") == 0) {
			spprintf(&dist, 0, GENE_ROUTER_CONTENT_FA, src);
		} else {
			spprintf(&dist, 0, GENE_ROUTER_CONTENT_FH, src);;
		}
	} else {
		spprintf(&dist, 0, GENE_ROUTER_CONTENT_FM, src);
	}
	return dist;
}

/** {{{ char get_router_content(char *content TSRMLS_DC)
 */
char * get_router_content(zval **content,char *method,char *path TSRMLS_DC)
{
	char *contents,*seg,*ptr,*tmp;
	spprintf(&contents, 0, "%s", Z_STRVAL_PP(content));
	seg = php_strtok_r(contents, "@", &ptr);
	if (seg && ptr && strlen(ptr)>0) {
		if (strcmp(method, "hook") == 0) {
			if (strcmp(path, "before") == 0) {
				spprintf(&tmp, 0, GENE_ROUTER_CONTENT_B, seg,ptr);
			} else if (strcmp(path, "after") == 0) {
				spprintf(&tmp, 0, GENE_ROUTER_CONTENT_A, seg,ptr);
			} else {
				spprintf(&tmp, 0, GENE_ROUTER_CONTENT_H, seg,ptr);
			}
		} else {
			spprintf(&tmp, 0, GENE_ROUTER_CONTENT_M, seg,ptr);
		}
		efree(contents);
		return tmp;
	}
	return NULL;
}

/*
 *  {{{ void get_router_content_run(char *methodin,char *pathin,zval *safe TSRMLS_DC)
 */
void get_router_content_run(char *methodin,char *pathin,zval *safe TSRMLS_DC)
{
	char *method = NULL,*path = NULL,*run = NULL,*hook = NULL,*router_e;
	int router_e_len;
	zval *server = NULL,** temp,**params,**lead;
	zval *cache = NULL,*cacheHook = NULL,*gene_url = NULL;

    if (methodin == NULL && pathin == NULL) {
    	server = request_query(TRACK_VARS_SERVER, NULL, 0 TSRMLS_CC);
    	if (server) {
        	if (zend_hash_find(HASH_OF(server), "REQUEST_METHOD", 15, (void **)&temp) == SUCCESS) {
        		spprintf(&method, 0, "%s", Z_STRVAL_PP(temp));
        		strtolower(method);
        	}
        	if (zend_hash_find(HASH_OF(server), "REQUEST_URI", 12, (void **)&temp) == SUCCESS) {
        		spprintf(&path, 0, "%s", Z_STRVAL_PP(temp));
        	}
    	}
    	server = NULL;
    } else {
    	spprintf(&method, 0, "%s", methodin);
    	strtolower(method);
    	spprintf(&path, 0, "%s", pathin);
    }

    if (method == NULL) {
        php_printf("Gene Unknown method:%s" , method);
        return;
    }
	if (safe != NULL && Z_STRLEN_P(safe)) {
		router_e_len = spprintf(&router_e, 0, "%s%s", Z_STRVAL_P(safe), GENE_ROUTER_ROUTER_TREE);
	} else {
		router_e_len = spprintf(&router_e, 0, "%s", GENE_ROUTER_ROUTER_TREE);
	}
    cache = gene_cache_get_quick(router_e, router_e_len TSRMLS_CC);
    efree(router_e);
	if (cache) {
		if (zend_hash_find(cache->value.ht, method, strlen(method)+1, (void **)&temp) == FAILURE){
			php_printf("Gene Unknown method:%s" , method);
			efree(method);
			efree(path);
			// zval_ptr_dtor(&cache);
			cache = NULL;
			return;
		}
		if (safe != NULL && Z_STRLEN_P(safe)) {
			router_e_len = spprintf(&router_e, 0, "%s%s", Z_STRVAL_P(safe), GENE_ROUTER_ROUTER_EVENT);
		} else {
			router_e_len = spprintf(&router_e, 0, "%s", GENE_ROUTER_ROUTER_EVENT);
		}
		cacheHook = gene_cache_get_quick(router_e, router_e_len TSRMLS_CC);
		efree(router_e);
		trim(path, '/');
		replaceAll(path,'.','/');
		if (zend_hash_find(&EG(symbol_table), "gene_url", 9, (void **)&params) == FAILURE){
			MAKE_STD_ZVAL(gene_url);
			array_init(gene_url);
			zend_hash_add(&EG(symbol_table),"gene_url", 9, &gene_url, sizeof(zval *), NULL);
		} else {
			gene_url = *params;
		}

		lead = get_path_router(temp,path,gene_url TSRMLS_CC);
		if (lead) {
			get_router_info(lead,&cacheHook TSRMLS_CC);
			lead = NULL;
		} else {
			php_printf("Gene Unknown method:%s" , path);
		}
		cache = NULL;
		//zval_ptr_dtor(&cache);
		if (cacheHook) {
			//zval_ptr_dtor(&cacheHook);
			cacheHook = NULL;
		}
	} else {
		php_printf("Gene Unknown method:%s" , path);
	}
	efree(method);
	efree(path);
	temp = NULL;
	return;
}

/*
 * {{{ gene_router_methods
 */
PHP_METHOD(gene_router, run)
{
	char *methodin = NULL,*pathin = NULL;
	int methodlen,pathlen;
	zval *safe;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"|ss", &methodin, &methodlen,&pathin, &pathlen) == FAILURE)
    {
        RETURN_NULL();
    }
	safe = zend_read_property(gene_router_ce, getThis(), GENE_ROUTER_SAFE, strlen(GENE_ROUTER_SAFE), 1 TSRMLS_CC);
	get_router_content_run(methodin,pathin,safe TSRMLS_CC);
	RETURN_TRUE;
}
/* }}} */

/*
 * {{{ gene_router_methods
 */
PHP_METHOD(gene_router, __construct)
{
	zval *safe = NULL;
	int len = 0;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"|z", &safe) == FAILURE)
    {
        RETURN_NULL();
    }
    if(safe) {
    	zend_update_property_string(gene_router_ce, getThis(), GENE_ROUTER_SAFE, strlen(GENE_ROUTER_SAFE), Z_STRVAL_P(safe) TSRMLS_CC);
    } else {
    	if (GENE_G(app_key)) {
    		zend_update_property_string(gene_router_ce, getThis(), GENE_ROUTER_SAFE, strlen(GENE_ROUTER_SAFE), GENE_G(app_key) TSRMLS_CC);
    	}
    }
}
/* }}} */

char * str_add1(const char *s, int length)
{
	char *p;
#ifdef ZEND_SIGNALS
	TSRMLS_FETCH();
#endif

	HANDLE_BLOCK_INTERRUPTIONS();

	p = (char *) ecalloc(length+1,sizeof(char));
	if (UNEXPECTED(p == NULL)) {
		HANDLE_UNBLOCK_INTERRUPTIONS();
		return p;
	}
	if (length) {
		memcpy(p, s, length);
	}
	p[length] = 0;
	HANDLE_UNBLOCK_INTERRUPTIONS();
	return p;
}

/*
 * {{{ public gene_router::GetOpcodes($codeString)
 */
PHP_METHOD(gene_router, __call)
{
	zval *val = NULL,*content = NULL,*prefix,*safe,*self = getThis();
	zval **pathVal,**contentval,**hook = NULL;
	int methodlen,i,router_e_len;
	char *result=NULL,*tmp = NULL,*router_e,*key,*method,*path = NULL;
	const char *methods[9]= {"get","post","put","patch","delete","trace","connect","options","head"},*event[2]= {"hook","error"},*common[2]= {"group","prefix"};

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz", &method, &methodlen, &val) == FAILURE) {
		RETURN_NULL();
	} else {
		strtolower(method);
		if (IS_ARRAY == Z_TYPE_P(val)) {
		    if(zend_hash_index_find(val->value.ht, 0, (void **)&pathVal) == SUCCESS) {
		    	convert_to_string(*pathVal);
		    	//paths = estrndup((*path)->value.str.val,(*path)->value.str.len);
			    for(i=0;i<9;i++) {
			    	if (strcmp(methods[i], method)==0) {
			    		prefix = zend_read_property(gene_router_ce, self, GENE_ROUTER_PREFIX, strlen(GENE_ROUTER_PREFIX), 1 TSRMLS_CC);
			    		spprintf(&path, 0, "%s%s", Z_STRVAL_P(prefix), Z_STRVAL_PP(pathVal));
			    		break;
			    	}
			    }
		    	if (path == NULL) {
		    		spprintf(&path, 0, "%s", Z_STRVAL_PP(pathVal));
		    	}
		    }
		    if(zend_hash_index_find(val->value.ht, 1, (void **)&contentval) == SUCCESS) {
		    	MAKE_STD_ZVAL(content);
		    	if (IS_OBJECT == Z_TYPE_PP(contentval)) {
		    		tmp = get_function_content(contentval TSRMLS_CC);
	    			result = get_router_content_F(tmp,method,path TSRMLS_CC);
	    			efree(tmp);
	    			tmp = NULL;
		    	} else {
		    		result = get_router_content(contentval,method,path TSRMLS_CC);
		    	}
	    		if (result) {
		    		ZVAL_STRING(content,result,1);
		    		efree(result);
	    		} else {
	    			ZVAL_STRING(content,"",1);
	    		}
		    }

		    if(zend_hash_index_find(val->value.ht, 2, (void **)&hook) == SUCCESS) {
		    	convert_to_string(*hook);
		    }

		    //call tree
		    for(i=0;i<9;i++) {
		    	if (strcmp(methods[i], method)==0) {
		    		trim(path, '/');
		    		safe = zend_read_property(gene_router_ce, self, GENE_ROUTER_SAFE, strlen(GENE_ROUTER_SAFE), 1 TSRMLS_CC);
		    		if (Z_STRLEN_P(safe)) {
		    			router_e_len = spprintf(&router_e, 0, "%s%s", Z_STRVAL_P(safe), GENE_ROUTER_ROUTER_TREE);
		    		} else {
		    			router_e_len = spprintf(&router_e, 0, "%s", GENE_ROUTER_ROUTER_TREE);
		    		}
		    		if (strlen(path) == 0) {
			    		spprintf(&key, 0, GENE_ROUTER_LEAF_RUN, method);
			    		gene_cache_set_by_router(router_e, router_e_len, key, content, 0 TSRMLS_CC);
			    		efree(key);
			    		if (hook) {
				    		spprintf(&key, 0, GENE_ROUTER_LEAF_HOOK, method);
				    		gene_cache_set_by_router(router_e,router_e_len, key, *hook, 0 TSRMLS_CC);
				    		efree(key);
			    		}
		    		} else {
		    			replaceAll(path,'.','/');
		    			tmp = replace_string (path, ':', GENE_ROUTER_CHIRD);
		    			if (tmp == NULL) {
				    		spprintf(&key, 0, GENE_ROUTER_LEAF_RUN_L, method,path);
				    		gene_cache_set_by_router(router_e, router_e_len, key,content, 0 TSRMLS_CC);
				    		efree(key);
				    		if (hook) {
					    		spprintf(&key, 0, GENE_ROUTER_LEAF_HOOK_L, method,path);
					    		gene_cache_set_by_router(router_e, router_e_len, key, *hook, 0 TSRMLS_CC);
					    		efree(key);
				    		}
		    			} else {
				    		spprintf(&key, 0, GENE_ROUTER_LEAF_RUN_L, method,tmp);
				    		gene_cache_set_by_router(router_e, router_e_len, key, content, 0 TSRMLS_CC);
				    		efree(key);
				    		if (hook) {
					    		spprintf(&key, 0, GENE_ROUTER_LEAF_HOOK_L, method,tmp);
					    		gene_cache_set_by_router(router_e, router_e_len, key, *hook, 0 TSRMLS_CC);
					    		efree(key);
				    		}
				    		efree(tmp);
		    			}
		    		}
		    		efree(router_e);
		    		efree(path);
		    		zval_ptr_dtor(&val);
		    		zval_ptr_dtor(&content);
		    		RETURN_ZVAL(self, 1, 0);
		    	}
		    }

		    //call event
		    for(i=0;i<2;i++) {
		    	if (strcmp(event[i], method)==0) {
		    		safe = zend_read_property(gene_router_ce, self, GENE_ROUTER_SAFE, strlen(GENE_ROUTER_SAFE), 1 TSRMLS_CC);
		    		if (Z_STRLEN_P(safe)) {
		    			router_e_len = spprintf(&router_e, 0, "%s%s", Z_STRVAL_P(safe), GENE_ROUTER_ROUTER_EVENT);
		    		} else {
		    			router_e_len = spprintf(&router_e, 0, "%s", GENE_ROUTER_ROUTER_EVENT);
		    		}
		    		spprintf(&key, 0, "%s:%s", method,path);
		    		gene_cache_set_by_router(router_e, router_e_len, key, content, 0 TSRMLS_CC);
		    		efree(router_e);
		    		efree(key);
		    		efree(path);
		    		zval_ptr_dtor(&val);
		    		zval_ptr_dtor(&content);
		    		RETURN_ZVAL(self, 1, 0);
		    	}
		    }
		    //call common
		    for(i=0;i<2;i++) {
		    	if (strcmp(common[i], method)==0) {
		    		if (path == NULL) {
		    			zend_update_property_string(gene_router_ce, self, GENE_ROUTER_PREFIX, strlen(GENE_ROUTER_PREFIX), "" TSRMLS_CC);
		    		} else {
		    			zend_update_property_string(gene_router_ce, self, GENE_ROUTER_PREFIX, strlen(GENE_ROUTER_PREFIX), path TSRMLS_CC);
		    		}
		    		efree(path);
		    		if (val) {
		    			zval_ptr_dtor(&val);
		    			val = NULL;
		    		}
		    		if (content) {
		    			zval_ptr_dtor(&content);
		    			content = NULL;
		    		}
		    		RETURN_ZVAL(self, 1, 0);
		    	}
		    }
    		if (val) {
    			zval_ptr_dtor(&val);
    			val = NULL;
    		}
		}
		RETURN_ZVAL(self, 1, 0);
	}
}
/* }}} */

/*
 * {{{ public gene_router::getEvent()
 */
PHP_METHOD(gene_router, getEvent)
{
	zval *self = getThis(),*safe,*cache = NULL;
	int router_e_len;
	char *router_e;
	safe = zend_read_property(gene_router_ce, self, GENE_ROUTER_SAFE, strlen(GENE_ROUTER_SAFE), 1 TSRMLS_CC);
	if (Z_STRLEN_P(safe)) {
		router_e_len = spprintf(&router_e, 0, "%s%s", Z_STRVAL_P(safe), GENE_ROUTER_ROUTER_EVENT);
	} else {
		router_e_len = spprintf(&router_e, 0, "%s", GENE_ROUTER_ROUTER_EVENT);
	}
    cache = gene_cache_get(router_e, router_e_len TSRMLS_CC);
    efree(router_e);
    if (cache) {
    	RETURN_ZVAL(cache, 1, 1);
    }
	RETURN_NULL();
}
/* }}} */


/*
 * {{{ public gene_router::getTree()
 */
PHP_METHOD(gene_router, getTree)
{
	zval *self = getThis(),*safe,*cache = NULL;
	int router_e_len;
	char *router_e;
	safe = zend_read_property(gene_router_ce, self, GENE_ROUTER_SAFE, strlen(GENE_ROUTER_SAFE), 1 TSRMLS_CC);
	if (Z_STRLEN_P(safe)) {
		router_e_len = spprintf(&router_e, 0, "%s%s", Z_STRVAL_P(safe), GENE_ROUTER_ROUTER_TREE);
	} else {
		router_e_len = spprintf(&router_e, 0, "%s", GENE_ROUTER_ROUTER_TREE);
	}
    cache = gene_cache_get(router_e, router_e_len TSRMLS_CC);
    efree(router_e);
    if (cache) {
    	RETURN_ZVAL(cache, 1, 1);
    }
	RETURN_NULL();
}
/* }}} */

/*
 * {{{ public gene_router::delTree()
 */
PHP_METHOD(gene_router, delTree)
{
	zval *self = getThis(),*safe;
	int router_e_len,ret;
	char *router_e;
	safe = zend_read_property(gene_router_ce, self, GENE_ROUTER_SAFE, strlen(GENE_ROUTER_SAFE), 1 TSRMLS_CC);
	if (Z_STRLEN_P(safe)) {
		router_e_len = spprintf(&router_e, 0, "%s%s", Z_STRVAL_P(safe), GENE_ROUTER_ROUTER_TREE);
	} else {
		router_e_len = spprintf(&router_e, 0, "%s", GENE_ROUTER_ROUTER_TREE);
	}
	ret = gene_cache_del(router_e, router_e_len TSRMLS_CC);
	if (ret) {
		efree(router_e);
		RETURN_TRUE;
	}
	efree(router_e);
	RETURN_FALSE;
}
/* }}} */

/*
 * {{{ public gene_router::delTree()
 */
PHP_METHOD(gene_router, delEvent)
{
	zval *self = getThis(),*safe;
	int router_e_len,ret;
	char *router_e;
	safe = zend_read_property(gene_router_ce, self, GENE_ROUTER_SAFE, strlen(GENE_ROUTER_SAFE), 1 TSRMLS_CC);
	if (Z_STRLEN_P(safe)) {
		router_e_len = spprintf(&router_e, 0, "%s%s", Z_STRVAL_P(safe), GENE_ROUTER_ROUTER_EVENT);
	} else {
		router_e_len = spprintf(&router_e, 0, "%s", GENE_ROUTER_ROUTER_EVENT);
	}
	ret = gene_cache_del(router_e, router_e_len TSRMLS_CC);
	if (ret) {
		efree(router_e);
		RETURN_TRUE;
	}
	efree(router_e);
	RETURN_FALSE;
}
/* }}} */

/*
 * {{{ public gene_router::clear()
 */
PHP_METHOD(gene_router, clear)
{
	zval *self = getThis(),*safe;
	int router_e_len,ret;
	char *router_e;
	safe = zend_read_property(gene_router_ce, self, GENE_ROUTER_SAFE, strlen(GENE_ROUTER_SAFE), 1 TSRMLS_CC);
	if (Z_STRLEN_P(safe)) {
		router_e_len = spprintf(&router_e, 0, "%s%s", Z_STRVAL_P(safe), GENE_ROUTER_ROUTER_TREE);
	} else {
		router_e_len = spprintf(&router_e, 0, "%s", GENE_ROUTER_ROUTER_TREE);
	}
	ret = gene_cache_del(router_e, router_e_len TSRMLS_CC);
	if (ret) {
		efree(router_e);
	}
	if (Z_STRLEN_P(safe)) {
		router_e_len = spprintf(&router_e, 0, "%s%s", Z_STRVAL_P(safe), GENE_ROUTER_ROUTER_EVENT);
	} else {
		router_e_len = spprintf(&router_e, 0, "%s", GENE_ROUTER_ROUTER_EVENT);
	}
	ret = gene_cache_del(router_e, router_e_len TSRMLS_CC);
	if (ret) {
		efree(router_e);
	}
	RETURN_ZVAL(self, 1, 0);
}
/* }}} */

/*
 * {{{ public gene_router::getTime()
 */
PHP_METHOD(gene_router, getTime)
{
	zval *self = getThis(),*safe;
	int router_e_len;
	long ctime = 0;
	char *router_e;
	safe = zend_read_property(gene_router_ce, self, GENE_ROUTER_SAFE, strlen(GENE_ROUTER_SAFE), 1 TSRMLS_CC);
	if (Z_STRLEN_P(safe)) {
		router_e_len = spprintf(&router_e, 0, "%s%s", Z_STRVAL_P(safe), GENE_ROUTER_ROUTER_TREE);
	} else {
		router_e_len = spprintf(&router_e, 0, "%s", GENE_ROUTER_ROUTER_TREE);
	}
	ctime = gene_cache_getTime(router_e, router_e_len TSRMLS_CC);
	efree(router_e);
	if (ctime > 0) {
		ctime = time(NULL) - ctime;
		RETURN_LONG(ctime);
	}
	RETURN_NULL();
}
/* }}} */

/*
 * {{{ public gene_router::getRouter()
 */
PHP_METHOD(gene_router, getRouter)
{
	zval *self = getThis();
	RETURN_ZVAL(self, 1, 0);
}
/* }}} */


/*
 * {{{ gene_router_methods
 */
zend_function_entry gene_router_methods[] = {
		PHP_ME(gene_router, getEvent, NULL, ZEND_ACC_PUBLIC)
		PHP_ME(gene_router, getTree, NULL, ZEND_ACC_PUBLIC)
		PHP_ME(gene_router, delTree, NULL, ZEND_ACC_PUBLIC)
		PHP_ME(gene_router, delEvent, NULL, ZEND_ACC_PUBLIC)
		PHP_ME(gene_router, clear, NULL, ZEND_ACC_PUBLIC)
		PHP_ME(gene_router, getTime, NULL, ZEND_ACC_PUBLIC)
		PHP_ME(gene_router, getRouter, NULL, ZEND_ACC_PUBLIC)
		PHP_ME(gene_router, run, NULL, ZEND_ACC_PUBLIC)
		PHP_ME(gene_router, __call, gene_router_call_arginfo, ZEND_ACC_PUBLIC)
		PHP_ME(gene_router, __construct, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
		{NULL, NULL, NULL}
};
/* }}} */


/*
 * {{{ GENE_MINIT_FUNCTION
 */
GENE_MINIT_FUNCTION(router)
{
    zend_class_entry gene_router;
    INIT_CLASS_ENTRY(gene_router,"gene_router",gene_router_methods);
    gene_router_ce = zend_register_internal_class(&gene_router TSRMLS_CC);

	//prop
    zend_declare_property_string(gene_router_ce, GENE_ROUTER_SAFE, strlen(GENE_ROUTER_SAFE), "", ZEND_ACC_PUBLIC TSRMLS_CC);
    zend_declare_property_string(gene_router_ce, GENE_ROUTER_PREFIX, strlen(GENE_ROUTER_PREFIX), "", ZEND_ACC_PUBLIC TSRMLS_CC);
    //
	return SUCCESS;
}
/* }}} */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
