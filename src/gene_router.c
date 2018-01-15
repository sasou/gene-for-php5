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
#include "Zend/zend_interfaces.h"
#include "ext/pcre/php_pcre.h"

#include "php_gene.h"
#include "gene_router.h"
#include "gene_cache.h"
#include "gene_common.h"
#include "gene_application.h"
#include "gene_view.h"

zend_class_entry *gene_router_ce;
static zend_class_entry **reflection_function_ptr;
static zend_class_entry **spl_ce_SplFileObject;

/** {{{ ARG_INFO
 */
ZEND_BEGIN_ARG_INFO_EX(gene_router_call_arginfo, 0, 0, 2) ZEND_ARG_INFO(0, name)
ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()
/* }}} */

/** {{{ static void get_router_info(char *keyString, int keyString_len TSRMLS_DC)
 */
void set_mvc(char *key, int len, char *val TSRMLS_DC) {
	if (len == 2) {
		if (strcmp(key, "c") == 0) {
			GENE_G(controller) = estrdup(val);
			return;
		}
		if (strcmp(key, "a") == 0) {
			GENE_G(action) = estrdup(val);
			return;
		}
	}
}

/** {{{ static void get_path_router(char *keyString, int keyString_len TSRMLS_DC)
 */
zval ** get_path_router(zval **val, char *paths TSRMLS_DC) {
	zval **ret = NULL, **tmp = NULL, **leaf = NULL, *var;
	char *seg = NULL, *ptr = NULL, *key = NULL, *path = NULL;
	int keylen;
	long idx;
	if (strlen(paths) == 0) {
		if (zend_hash_find((*val)->value.ht, "leaf", 5,
				(void **) &leaf) == FAILURE) {
			return NULL;
		}
		return leaf;
	} else {
		spprintf(&path, 0, "%s", paths);
		seg = php_strtok_r(path, "/", &ptr);
		if (ptr && strlen(seg) > 0) {
			if (zend_hash_find((*val)->value.ht, seg, strlen(seg) + 1,
					(void **) &ret) == SUCCESS) {
				leaf = get_path_router(ret, ptr TSRMLS_CC);
			} else {
				if (zend_hash_find((*val)->value.ht, "chird", 6,
						(void **) &ret) == SUCCESS) {
					for (zend_hash_internal_pointer_reset((*ret)->value.ht);
							zend_hash_has_more_elements((*ret)->value.ht)
									== SUCCESS;
							zend_hash_move_forward((*ret)->value.ht)) {
						if (zend_hash_get_current_key_ex((*ret)->value.ht, &key,
								&keylen, &idx, 0, NULL) == HASH_KEY_IS_LONG) {
							if (zend_hash_get_current_data((*ret)->value.ht,
									(void** )&tmp) == SUCCESS) {
								leaf = get_path_router(tmp, ptr TSRMLS_CC);
								if (leaf) {
									break;
								}
							}

						} else {
							if (zend_hash_get_current_data((*ret)->value.ht,
									(void** )&tmp) == SUCCESS) {
								leaf = get_path_router(tmp, ptr TSRMLS_CC);
								if (leaf) {
									set_mvc(key, keylen, seg TSRMLS_CC);
									MAKE_STD_ZVAL(var);
									ZVAL_STRING(var, seg, 1);
									zend_hash_update(GENE_G(params), key,
											keylen + 1, (void ** )&var,
											sizeof(zval *), NULL);
									break;
								}
							}
						}
					}
				}
			}
		} else {
			if (zend_hash_find((*val)->value.ht, seg, strlen(seg) + 1,
					(void **) &ret) == SUCCESS) {
				if (zend_hash_find((*ret)->value.ht, "leaf", 5,
						(void **) &leaf) == FAILURE) {
					leaf = NULL;
				}
			} else {
				if (zend_hash_find((*val)->value.ht, "chird", 6,
						(void **) &ret) == SUCCESS) {
					for (zend_hash_internal_pointer_reset((*ret)->value.ht);
							zend_hash_has_more_elements((*ret)->value.ht)
									== SUCCESS;
							zend_hash_move_forward((*ret)->value.ht)) {
						if (zend_hash_get_current_key_ex((*ret)->value.ht, &key,
								&keylen, &idx, 0, NULL) == HASH_KEY_IS_LONG) {
							if (zend_hash_get_current_data((*ret)->value.ht,
									(void** )&tmp) == SUCCESS) {
								if (zend_hash_find((*tmp)->value.ht, "leaf", 5,
										(void **) &leaf) == SUCCESS) {
									break;
								}
							}
						} else {
							if (zend_hash_get_current_data((*ret)->value.ht,
									(void** )&tmp) == SUCCESS) {
								if (zend_hash_find((*tmp)->value.ht, "leaf", 5,
										(void **) &leaf) == SUCCESS) {
									set_mvc(key, keylen, seg TSRMLS_CC);
									MAKE_STD_ZVAL(var);
									ZVAL_STRING(var, seg, 1);
									zend_hash_update(GENE_G(params), key,
											keylen + 1, (void ** )&var,
											sizeof(zval *), NULL);
									break;
								}
							}
						}
					}
				}
			}
		}
	}
	efree(path);
	path = NULL;
	return leaf;
}
/* }}} */

/** {{{ static void get_router_info(char *keyString, int keyString_len TSRMLS_DC)
 */
int get_router_info(zval **leaf, zval **cacheHook TSRMLS_DC) {
	zval **hname, **before, **after, **m, **h;
	int size = 0, is = 1;
	char *run = NULL, *hookname = NULL, *seg = NULL, *ptr = NULL;
	gene_router_set_uri(leaf TSRMLS_CC);
	if (zend_hash_find((*leaf)->value.ht, "hook", 5,
			(void **) &hname) == SUCCESS) {
		spprintf(&hookname, 0, "hook:%s", Z_STRVAL_PP(hname));
	}
	if (hookname) {
		seg = php_strtok_r(hookname, "@", &ptr);
	}
	size = strlen(GENE_ROUTER_CHIRD_PRE) + 1;
	run = (char *) ecalloc(size, sizeof(char));
	strcat(run, GENE_ROUTER_CHIRD_PRE);
	run[size - 1] = 0;
	if (ptr && strlen(ptr) > 0) {
		if ((strcmp(ptr, "clearBefore") == 0)
				|| (strcmp(ptr, "clearAll") == 0)) {
			is = 0;
		}
	}
	if (is
			== 1&& zend_hash_find((*cacheHook)->value.ht, "hook:before", 12, (void **)&before) == SUCCESS) {
		size = size + Z_STRLEN_PP(before);
		run = erealloc(run, size);
		strcat(run, Z_STRVAL_PP(before));
		run[size - 1] = 0;
	}
	is = 1;
	if (seg && strlen(seg) > 0) {
		if (zend_hash_find((*cacheHook)->value.ht, seg, strlen(seg) + 1,
				(void **) &h) == SUCCESS) {
			size = size + Z_STRLEN_PP(h);
			run = erealloc(run, size);
			strcat(run, Z_STRVAL_PP(h));
			run[size - 1] = 0;
		}
	}
	if (zend_hash_find((*leaf)->value.ht, "run", 4, (void **) &m) == SUCCESS) {
		//todo 替换路由
		//StrReplace();
		size = size + Z_STRLEN_PP(m);
		run = erealloc(run, size);
		strcat(run, Z_STRVAL_PP(m));
		run[size - 1] = 0;
	}
	if (ptr && strlen(ptr) > 0) {
		if ((strcmp(ptr, "clearAfter") == 0)
				|| (strcmp(ptr, "clearAll") == 0)) {
			is = 0;
		}
	}
	if (is
			== 1&& zend_hash_find((*cacheHook)->value.ht, "hook:after", 11, (void **)&after) == SUCCESS) {
		size = size + Z_STRLEN_PP(after);
		run = erealloc(run, size);
		strcat(run, Z_STRVAL_PP(after));
		run[size - 1] = 0;
	}
	zend_try
			{
				zend_eval_stringl(run, strlen(run), NULL, "" TSRMLS_CC);
			}zend_catch
			{
				efree(run);
				run = NULL;
				if (hookname) {
					efree(hookname);
					hookname = NULL;
				}
				zend_bailout();
			}zend_end_try();
	efree(run);
	run = NULL;
	if (hookname) {
		efree(hookname);
		hookname = NULL;
	}
	return 1;
}
/* }}} */

/** {{{ static void get_router_info(char *keyString, int keyString_len TSRMLS_DC)
 */
int get_router_error_run_by_router(zval *cacheHook, char *errorName TSRMLS_DC) {
	zval **error = NULL;
	int router_e_len;
	char *run = NULL, *router_e;
	if (cacheHook) {
		router_e_len = spprintf(&router_e, 0, "error:%s", errorName);
		if (zend_hash_find(cacheHook->value.ht, router_e, router_e_len + 1,
				(void **) &error) == SUCCESS) {
			spprintf(&run, 0, "%s%s", GENE_ROUTER_CHIRD_PRE,
					Z_STRVAL_PP(error));
			zend_try
					{
						zend_eval_stringl(run, strlen(run), NULL,
								errorName TSRMLS_CC);
					}zend_catch
					{
						efree(router_e);
						efree(run);
						run = NULL;
						zend_bailout();
					}zend_end_try();
			efree(router_e);
			efree(run);
			run = NULL;
			return 1;
		}
		efree(router_e);
		return 0;
	}
	return 0;
}
/* }}} */

/** {{{ static void get_router_info(char *keyString, int keyString_len TSRMLS_DC)
 */
int get_router_error_run(char *errorName, zval *safe TSRMLS_DC) {
	zval *cacheHook = NULL, **error = NULL;
	int router_e_len;
	char *run = NULL, *router_e;
	if (safe != NULL && Z_STRLEN_P(safe)) {
		router_e_len = spprintf(&router_e, 0, "%s%s", Z_STRVAL_P(safe),
				GENE_ROUTER_ROUTER_EVENT);
	} else {
		router_e_len = spprintf(&router_e, 0, "%s", GENE_ROUTER_ROUTER_EVENT);
	}
	cacheHook = gene_cache_get_quick(router_e, router_e_len TSRMLS_CC);
	efree(router_e);
	if (cacheHook) {
		router_e_len = spprintf(&router_e, 0, "error:%s", errorName);
		if (zend_hash_find(cacheHook->value.ht, router_e, router_e_len + 1,
				(void **) &error) == SUCCESS) {
			if (error) {
				spprintf(&run, 0, "%s%s", GENE_ROUTER_CHIRD_PRE,
						Z_STRVAL_PP(error));
			}
		} else {
			efree(router_e);
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Gene Unknown Error:%s",
					errorName);
			return 0;
		}
		efree(router_e);
	} else {
		return 0;
	}
	zend_try
			{
				zend_eval_stringl(run, strlen(run), NULL, "" TSRMLS_CC);
			}zend_catch
			{
				efree(run);
				run = NULL;
				zend_bailout();
			}zend_end_try();
	efree(run);
	run = NULL;
	return 1;
}
/* }}} */

/** {{{ static void get_function_content(char *keyString, int keyString_len TSRMLS_DC)
 */
char * get_function_content(zval **content TSRMLS_DC) {
	zval *objEx, *ret, *fileName, *arg, *arg1;
	int startline, endline, size;
	zval *params[3];
	char *result = NULL, *tmp = NULL;

	MAKE_STD_ZVAL(objEx);
	MAKE_STD_ZVAL(ret);
	MAKE_STD_ZVAL(arg);
	_object_init_ex(objEx,
			*reflection_function_ptr ZEND_FILE_LINE_CC TSRMLS_CC);
	ZVAL_STRING(arg, "__construct", 1);
	params[0] = *content;
	call_user_function(NULL, &objEx, arg, ret, 1, params TSRMLS_CC);
	zval_ptr_dtor(&ret);
	zval_ptr_dtor(&arg);

	MAKE_STD_ZVAL(arg);
	MAKE_STD_ZVAL(fileName);
	ZVAL_STRING(arg, "getFileName", 1);
	call_user_function(NULL, &objEx, arg, fileName, 0, NULL TSRMLS_CC);
	zval_ptr_dtor(&arg);

	MAKE_STD_ZVAL(ret);
	MAKE_STD_ZVAL(arg);
	ZVAL_STRING(arg, "getStartLine", 1);
	call_user_function(NULL, &objEx, arg, ret, 0, NULL TSRMLS_CC);
	startline = Z_LVAL_P(ret) - 1;
	zval_ptr_dtor(&ret);
	zval_ptr_dtor(&arg);

	MAKE_STD_ZVAL(ret);
	MAKE_STD_ZVAL(arg);
	ZVAL_STRING(arg, "getEndLine", 1);
	call_user_function(NULL, &objEx, arg, ret, 0, NULL TSRMLS_CC);
	endline = Z_LVAL_P(ret);
	zval_ptr_dtor(&ret);
	zval_ptr_dtor(&arg);
	zval_ptr_dtor(&objEx);

	//get codestartline
	MAKE_STD_ZVAL(objEx);
	MAKE_STD_ZVAL(ret);
	MAKE_STD_ZVAL(arg);

	_object_init_ex(objEx, *spl_ce_SplFileObject ZEND_FILE_LINE_CC TSRMLS_CC);
	ZVAL_STRING(arg, "__construct", 1);
	params[0] = fileName;
	call_user_function(NULL, &objEx, arg, ret, 1, params TSRMLS_CC);
	zval_ptr_dtor(&ret);
	zval_ptr_dtor(&arg);
	zval_ptr_dtor(&fileName);

	MAKE_STD_ZVAL(ret);
	MAKE_STD_ZVAL(arg);
	MAKE_STD_ZVAL(fileName);
	ZVAL_STRING(arg, "seek", 1);
	ZVAL_LONG(fileName, startline);
	params[0] = fileName;
	call_user_function(NULL, &objEx, arg, ret, 1, params TSRMLS_CC);
	zval_ptr_dtor(&ret);
	zval_ptr_dtor(&arg);
	zval_ptr_dtor(&fileName);

	while (startline < endline) {
		MAKE_STD_ZVAL(ret);
		MAKE_STD_ZVAL(arg);
		ZVAL_STRING(arg, "current", 1);
		call_user_function(NULL, &objEx, arg, ret, 0, NULL TSRMLS_CC);
		spprintf(&tmp, 0, "%s", Z_STRVAL_P(ret));
		zval_ptr_dtor(&ret);
		zval_ptr_dtor(&arg);

		if (result) {
			size = strlen(result) + strlen(tmp) + 1;
			result = erealloc(result, size);
			strcat(result, tmp);
			result[size - 1] = 0;
		} else {
			size = strlen(tmp) + 1;
			result = ecalloc(size, sizeof(char));
			strcpy(result, tmp);
			result[size - 1] = 0;
		}
		efree(tmp);
		++startline;

		MAKE_STD_ZVAL(ret);
		MAKE_STD_ZVAL(arg);
		ZVAL_STRING(arg, "next", 1);
		call_user_function(NULL, &objEx, arg, ret, 0, NULL TSRMLS_CC);
		zval_ptr_dtor(&ret);
		zval_ptr_dtor(&arg);
	}
	zval_ptr_dtor(&objEx);

	MAKE_STD_ZVAL(ret);
	MAKE_STD_ZVAL(arg);
	ZVAL_STRING(arg, "strpos", 1);
	MAKE_STD_ZVAL(fileName);
	ZVAL_STRING(fileName, "function", 1);
	MAKE_STD_ZVAL(arg1);
	ZVAL_STRING(arg1, result, 1);
	params[0] = arg1;
	params[1] = fileName;
	call_user_function(NULL, NULL, arg, ret, 2, params TSRMLS_CC);
	startline = Z_LVAL_P(ret);
	zval_ptr_dtor(&ret);
	zval_ptr_dtor(&fileName);
	zval_ptr_dtor(&arg);

	MAKE_STD_ZVAL(ret);
	MAKE_STD_ZVAL(arg);
	ZVAL_STRING(arg, "strrpos", 1);
	MAKE_STD_ZVAL(fileName);
	ZVAL_STRING(fileName, "}", 1);
	params[1] = fileName;
	call_user_function(NULL, NULL, arg, ret, 2, params TSRMLS_CC);
	endline = Z_LVAL_P(ret);
	zval_ptr_dtor(&ret);
	zval_ptr_dtor(&fileName);
	zval_ptr_dtor(&arg);
	zval_ptr_dtor(&arg1);

	tmp = ecalloc(endline - startline + 2, sizeof(char));
	mid(tmp, result, endline - startline + 1, startline);
	if (result != NULL) {
		efree(result);
		result = NULL;
	}
	remove_extra_space(tmp);
	params[0] = NULL;
	params[1] = NULL;
	params[2] = NULL;
	return tmp;
}
/* }}} */

/** {{{ char * get_function_content_quik(zval **content TSRMLS_DC)
 */
char * get_function_content_quik(zval **content TSRMLS_DC) {

	return NULL;
}
/* }}} */

/** {{{ char get_router_content(char *content TSRMLS_DC)
 */
char * get_router_content_F(char *src, char *method, char *path TSRMLS_DC) {
	char *dist;
	if (strcmp(method, "hook") == 0) {
		if (strcmp(path, "before") == 0) {
			spprintf(&dist, 0, GENE_ROUTER_CONTENT_FB, src);
		} else if (strcmp(path, "after") == 0) {
			spprintf(&dist, 0, GENE_ROUTER_CONTENT_FA, src);
		} else {
			spprintf(&dist, 0, GENE_ROUTER_CONTENT_FH, src);
			;
		}
	} else {
		spprintf(&dist, 0, GENE_ROUTER_CONTENT_FM, src);
	}
	return dist;
}

/** {{{ char get_router_content(char *content TSRMLS_DC)
 */
char * get_router_content(zval **content, char *method, char *path TSRMLS_DC) {
	char *contents, *seg, *ptr, *tmp;
	spprintf(&contents, 0, "%s", Z_STRVAL_PP(content));
	seg = php_strtok_r(contents, "@", &ptr);
	if (seg && ptr && strlen(ptr) > 0) {
		if (strcmp(method, "hook") == 0) {
			if (strcmp(path, "before") == 0) {
				spprintf(&tmp, 0, GENE_ROUTER_CONTENT_B, seg, ptr);
			} else if (strcmp(path, "after") == 0) {
				spprintf(&tmp, 0, GENE_ROUTER_CONTENT_A, seg, ptr);
			} else {
				spprintf(&tmp, 0, GENE_ROUTER_CONTENT_H, seg, ptr);
			}
		} else {
			spprintf(&tmp, 0, GENE_ROUTER_CONTENT_M, seg, ptr);
		}
		efree(contents);
		return tmp;
	}
	return NULL;
}

/*
 *  {{{ void get_router_content_run(char *methodin,char *pathin,zval *safe TSRMLS_DC)
 */
void get_router_content_run(char *methodin, char *pathin, zval *safe TSRMLS_DC) {
	char *method = NULL, *path = NULL, *run = NULL, *hook = NULL, *router_e;
	int router_e_len;
	zval ** temp, **lead;
	zval *cache = NULL, *cacheHook = NULL;

	if (methodin == NULL && pathin == NULL) {
		if (GENE_G(method)) {
			spprintf(&method, 0, "%s", GENE_G(method));
		}
		if (GENE_G(path)) {
			spprintf(&path, 0, "%s", GENE_G(path));
		}
	} else {
		spprintf(&method, 0, "%s", methodin);
		strtolower(method);
		spprintf(&path, 0, "%s", pathin);
	}

	if (method == NULL || path == NULL) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING,
				"Gene Unknown Method And Url: NULL");
		return;
	}
	if (safe != NULL && Z_STRLEN_P(safe)) {
		router_e_len = spprintf(&router_e, 0, "%s%s", Z_STRVAL_P(safe),
				GENE_ROUTER_ROUTER_TREE);
	} else {
		router_e_len = spprintf(&router_e, 0, "%s", GENE_ROUTER_ROUTER_TREE);
	}
	cache = gene_cache_get_quick(router_e, router_e_len TSRMLS_CC);
	efree(router_e);
	if (cache) {
		if (zend_hash_find(cache->value.ht, method, strlen(method) + 1,
				(void **) &temp) == FAILURE) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING,
					"Gene Unknown Method Cache:%s", method);
			efree(method);
			efree(path);
			// zval_ptr_dtor(&cache);
			cache = NULL;
			return;
		}
		if (safe != NULL && Z_STRLEN_P(safe)) {
			router_e_len = spprintf(&router_e, 0, "%s%s", Z_STRVAL_P(safe),
					GENE_ROUTER_ROUTER_EVENT);
		} else {
			router_e_len = spprintf(&router_e, 0, "%s",
					GENE_ROUTER_ROUTER_EVENT);
		}
		cacheHook = gene_cache_get_quick(router_e, router_e_len TSRMLS_CC);
		efree(router_e);
		trim(path, '/');
		replaceAll(path, '.', '/');
		lead = get_path_router(temp, path TSRMLS_CC);
		if (lead) {
			get_router_info(lead, &cacheHook TSRMLS_CC);
			lead = NULL;
		} else {
			if (!get_router_error_run_by_router(cacheHook, "404" TSRMLS_CC)) {
				if (GENE_G(path)) {
					php_error_docref(NULL TSRMLS_CC, E_WARNING,
							"Gene Unknown Url:%s", GENE_G(path));
				} else {
					php_error_docref(NULL TSRMLS_CC, E_WARNING,
							"Gene Unknown Url:%s", path);
				}
			}
		}
		cache = NULL;
		//zval_ptr_dtor(&cache);
		if (cacheHook) {
			//zval_ptr_dtor(&cacheHook);
			cacheHook = NULL;
		}
	} else {
		php_error_docref(NULL TSRMLS_CC, E_WARNING,
				"Gene Unknown Router Cache");
	}
	efree(method);
	efree(path);
	temp = NULL;
	return;
}

char * str_add1(const char *s, int length) {
	char *p;
#ifdef ZEND_SIGNALS
	TSRMLS_FETCH();
#endif

	HANDLE_BLOCK_INTERRUPTIONS();

	p = (char *) ecalloc(length + 1, sizeof(char));
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
 * {{{ gene_router_methods
 */
PHP_METHOD(gene_router, run) {
	char *methodin = NULL, *pathin = NULL;
	int methodlen = 0, pathlen = 0;
	zval *self = getThis(), *safe = NULL, *safein = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|ss", &methodin,
			&methodlen, &pathin, &pathlen) == FAILURE) {
		RETURN_NULL()
		;
	}

	safe = zend_read_property(gene_router_ce, self, GENE_ROUTER_SAFE,
			strlen(GENE_ROUTER_SAFE), 1 TSRMLS_CC);
	MAKE_STD_ZVAL(safein);
	if (safe && Z_STRLEN_P(safe)) {
		ZVAL_STRING(safein, Z_STRVAL_P(safe), 1);
	} else {
		ZVAL_STRING(safein, "", 1);
	}
	get_router_content_run(methodin, pathin, safein TSRMLS_CC);
	if (safein) {
		zval_ptr_dtor(&safein);
		safein = NULL;
	}
	RETURN_NULL()
	;
}
/* }}} */

/*
 * {{{ gene_router_methods
 */
PHP_METHOD(gene_router, runError) {
	char *methodin = NULL;
	int methodlen;
	zval *safe = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &methodin,
			&methodlen) == FAILURE) {
		RETURN_NULL()
		;
	}
	MAKE_STD_ZVAL(safe);
	if (GENE_G(app_key)) {
		ZVAL_STRING(safe, GENE_G(app_key), 1);
	} else {
		ZVAL_STRING(safe, GENE_G(directory), 1);
	}
	get_router_error_run(methodin, safe TSRMLS_CC);
	if (safe) {
		zval_ptr_dtor(&safe);
		safe = NULL;
	}
	RETURN_TRUE
	;
}
/* }}} */

/*
 * {{{ gene_router_methods
 */
PHP_METHOD(gene_router, __construct) {
	zval *safe = NULL;
	int len = 0, retval;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|z", &safe) == FAILURE) {
		RETURN_NULL()
		;
	}
	gene_ini_router(TSRMLS_C);
	if (safe) {
		zend_update_property_string(gene_router_ce, getThis(), GENE_ROUTER_SAFE,
				strlen(GENE_ROUTER_SAFE), Z_STRVAL_P(safe) TSRMLS_CC);
	} else {
		if (GENE_G(app_key)) {
			zend_update_property_string(gene_router_ce, getThis(),
					GENE_ROUTER_SAFE, strlen(GENE_ROUTER_SAFE),
					GENE_G(app_key) TSRMLS_CC);
		} else {
			zend_update_property_string(gene_router_ce, getThis(),
					GENE_ROUTER_SAFE, strlen(GENE_ROUTER_SAFE),
					GENE_G(directory) TSRMLS_CC);
		}
	}
	retval = zend_lookup_class("ReflectionFunction",
			sizeof("ReflectionFunction") - 1,
			&reflection_function_ptr TSRMLS_CC);
	if (FAILURE == retval) {
		php_error_docref(NULL, E_WARNING, "Unable to start ReflectionFunction");
	}
	zend_lookup_class("SplFileObject", sizeof("SplFileObject") - 1,
			&spl_ce_SplFileObject TSRMLS_CC);
	if (FAILURE == retval) {
		php_error_docref(NULL, E_WARNING, "Unable to start SplFileObject");
	}
}
/* }}} */

/*
 * {{{ public gene_router::__call($codeString)
 */
PHP_METHOD(gene_router, __call) {
	zval *val = NULL, *content = NULL, *prefix, *safe, *self = getThis(),
			*pathval = NULL;
	zval **pathVal, **contentval, **hook = NULL;
	int methodlen, i, router_e_len;
	char *result = NULL, *tmp = NULL, *router_e, *key, *method, *path = NULL;
	const char *methods[9] = { "get", "post", "put", "patch", "delete", "trace",
			"connect", "options", "head" }, *event[2] = { "hook", "error" },
			*common[2] = { "group", "prefix" };

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz", &method,
			&methodlen, &val) == FAILURE) {
		RETURN_NULL()
		;
	} else {
		strtolower(method);
		if (IS_ARRAY == Z_TYPE_P(val)) {
			if (zend_hash_index_find(val->value.ht, 0,
					(void **) &pathVal) == SUCCESS) {
				convert_to_string(*pathVal);
				//paths = estrndup((*path)->value.str.val,(*path)->value.str.len);
				for (i = 0; i < 9; i++) {
					if (strcmp(methods[i], method) == 0) {
						prefix = zend_read_property(gene_router_ce, self,
								GENE_ROUTER_PREFIX, strlen(GENE_ROUTER_PREFIX),
								1 TSRMLS_CC);
						spprintf(&path, 0, "%s%s", Z_STRVAL_P(prefix),
								Z_STRVAL_PP(pathVal));
						break;
					}
				}
				if (path == NULL) {
					spprintf(&path, 0, "%s", Z_STRVAL_PP(pathVal));
				}
			}
			if (zend_hash_index_find(val->value.ht, 1,
					(void **) &contentval) == SUCCESS) {
				MAKE_STD_ZVAL(content);
				if (IS_OBJECT == Z_TYPE_PP(contentval)) {
					tmp = get_function_content(contentval TSRMLS_CC);
					result = get_router_content_F(tmp, method, path TSRMLS_CC);
					if (tmp != NULL) {
						efree(tmp);
						tmp = NULL;
					}
				} else {
					result = get_router_content(contentval, method,
							path TSRMLS_CC);
				}
				if (result) {
					ZVAL_STRING(content, result, 1);
					efree(result);
				} else {
					ZVAL_STRING(content, "", 1);
				}
			}

			if (zend_hash_index_find(val->value.ht, 2,
					(void **) &hook) == SUCCESS) {
				convert_to_string(*hook);
			}

			//call tree
			for (i = 0; i < 9; i++) {
				if (strcmp(methods[i], method) == 0) {
					trim(path, '/');
					MAKE_STD_ZVAL(pathval);
					if (path != NULL) {
						ZVAL_STRING(pathval, path, 1);
					} else {
						ZVAL_STRING(pathval, "", 1);
					}
					safe = zend_read_property(gene_router_ce, self,
							GENE_ROUTER_SAFE, strlen(GENE_ROUTER_SAFE),
							1 TSRMLS_CC);
					if (Z_STRLEN_P(safe)) {
						router_e_len = spprintf(&router_e, 0, "%s%s",
								Z_STRVAL_P(safe), GENE_ROUTER_ROUTER_TREE);
					} else {
						router_e_len = spprintf(&router_e, 0, "%s",
								GENE_ROUTER_ROUTER_TREE);
					}
					if (strlen(path) == 0) {
						spprintf(&key, 0, GENE_ROUTER_LEAF_KEY, method);
						gene_cache_set_by_router(router_e, router_e_len, key,
								pathval, 0 TSRMLS_CC);
						efree(key);
						spprintf(&key, 0, GENE_ROUTER_LEAF_RUN, method);
						gene_cache_set_by_router(router_e, router_e_len, key,
								content, 0 TSRMLS_CC);
						efree(key);
						if (hook) {
							spprintf(&key, 0, GENE_ROUTER_LEAF_HOOK, method);
							gene_cache_set_by_router(router_e, router_e_len,
									key, *hook, 0 TSRMLS_CC);
							efree(key);
						}
					} else {
						replaceAll(path, '.', '/');
						tmp = replace_string(path, ':', GENE_ROUTER_CHIRD);
						if (tmp == NULL) {
							spprintf(&key, 0, GENE_ROUTER_LEAF_KEY_L, method,
									path);
							gene_cache_set_by_router(router_e, router_e_len,
									key, pathval, 0 TSRMLS_CC);
							efree(key);
							spprintf(&key, 0, GENE_ROUTER_LEAF_RUN_L, method,
									path);
							gene_cache_set_by_router(router_e, router_e_len,
									key, content, 0 TSRMLS_CC);
							efree(key);
							if (hook) {
								spprintf(&key, 0, GENE_ROUTER_LEAF_HOOK_L,
										method, path);
								gene_cache_set_by_router(router_e, router_e_len,
										key, *hook, 0 TSRMLS_CC);
								efree(key);
							}
						} else {
							spprintf(&key, 0, GENE_ROUTER_LEAF_KEY_L, method,
									tmp);
							gene_cache_set_by_router(router_e, router_e_len,
									key, pathval, 0 TSRMLS_CC);
							efree(key);
							spprintf(&key, 0, GENE_ROUTER_LEAF_RUN_L, method,
									tmp);
							gene_cache_set_by_router(router_e, router_e_len,
									key, content, 0 TSRMLS_CC);
							efree(key);
							if (hook) {
								spprintf(&key, 0, GENE_ROUTER_LEAF_HOOK_L,
										method, tmp);
								gene_cache_set_by_router(router_e, router_e_len,
										key, *hook, 0 TSRMLS_CC);
								efree(key);
							}
							efree(tmp);
						}
					}
					efree(router_e);
					efree(path);
					if (pathval != NULL) {
						zval_ptr_dtor(&pathval);
					}
					zval_ptr_dtor(&val);
					zval_ptr_dtor(&content);
					RETURN_ZVAL(self, 1, 0);
				}
			}

			//call event
			for (i = 0; i < 2; i++) {
				if (strcmp(event[i], method) == 0) {
					safe = zend_read_property(gene_router_ce, self,
							GENE_ROUTER_SAFE, strlen(GENE_ROUTER_SAFE),
							1 TSRMLS_CC);
					if (Z_STRLEN_P(safe)) {
						router_e_len = spprintf(&router_e, 0, "%s%s",
								Z_STRVAL_P(safe), GENE_ROUTER_ROUTER_EVENT);
					} else {
						router_e_len = spprintf(&router_e, 0, "%s",
								GENE_ROUTER_ROUTER_EVENT);
					}
					spprintf(&key, 0, "%s:%s", method, path);
					gene_cache_set_by_router(router_e, router_e_len, key,
							content, 0 TSRMLS_CC);
					efree(router_e);
					efree(key);
					efree(path);
					zval_ptr_dtor(&val);
					zval_ptr_dtor(&content);
					RETURN_ZVAL(self, 1, 0);
				}
			}
			//call common
			for (i = 0; i < 2; i++) {
				if (strcmp(common[i], method) == 0) {
					if (path == NULL) {
						zend_update_property_string(gene_router_ce, self,
								GENE_ROUTER_PREFIX, strlen(GENE_ROUTER_PREFIX),
								"" TSRMLS_CC);
					} else {
						trim(path, '/');
						zend_update_property_string(gene_router_ce, self,
								GENE_ROUTER_PREFIX, strlen(GENE_ROUTER_PREFIX),
								path TSRMLS_CC);
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
PHP_METHOD(gene_router, getEvent) {
	zval *self = getThis(), *safe, *cache = NULL;
	int router_e_len;
	char *router_e;
	safe = zend_read_property(gene_router_ce, self, GENE_ROUTER_SAFE,
			strlen(GENE_ROUTER_SAFE), 1 TSRMLS_CC);
	if (Z_STRLEN_P(safe)) {
		router_e_len = spprintf(&router_e, 0, "%s%s", Z_STRVAL_P(safe),
				GENE_ROUTER_ROUTER_EVENT);
	} else {
		router_e_len = spprintf(&router_e, 0, "%s", GENE_ROUTER_ROUTER_EVENT);
	}
	cache = gene_cache_get(router_e, router_e_len TSRMLS_CC);
	efree(router_e);
	if (cache) {
		RETURN_ZVAL(cache, 1, 1);
	}
	RETURN_NULL()
	;
}
/* }}} */

/*
 * {{{ public gene_router::getTree()
 */
PHP_METHOD(gene_router, getTree) {
	zval *self = getThis(), *safe, *cache = NULL;
	int router_e_len;
	char *router_e;
	safe = zend_read_property(gene_router_ce, self, GENE_ROUTER_SAFE,
			strlen(GENE_ROUTER_SAFE), 1 TSRMLS_CC);
	if (Z_STRLEN_P(safe)) {
		router_e_len = spprintf(&router_e, 0, "%s%s", Z_STRVAL_P(safe),
				GENE_ROUTER_ROUTER_TREE);
	} else {
		router_e_len = spprintf(&router_e, 0, "%s", GENE_ROUTER_ROUTER_TREE);
	}
	cache = gene_cache_get(router_e, router_e_len TSRMLS_CC);
	efree(router_e);
	if (cache) {
		RETURN_ZVAL(cache, 1, 1);
	}
	RETURN_NULL()
	;
}
/* }}} */

/*
 * {{{ public gene_router::delTree()
 */
PHP_METHOD(gene_router, delTree) {
	zval *self = getThis(), *safe;
	int router_e_len, ret;
	char *router_e;
	safe = zend_read_property(gene_router_ce, self, GENE_ROUTER_SAFE,
			strlen(GENE_ROUTER_SAFE), 1 TSRMLS_CC);
	if (Z_STRLEN_P(safe)) {
		router_e_len = spprintf(&router_e, 0, "%s%s", Z_STRVAL_P(safe),
				GENE_ROUTER_ROUTER_TREE);
	} else {
		router_e_len = spprintf(&router_e, 0, "%s", GENE_ROUTER_ROUTER_TREE);
	}
	ret = gene_cache_del(router_e, router_e_len TSRMLS_CC);
	if (ret) {
		efree(router_e);
		RETURN_TRUE
		;
	}
	efree(router_e);
	RETURN_FALSE
	;
}
/* }}} */

/*
 * {{{ public gene_router::delTree()
 */
PHP_METHOD(gene_router, delEvent) {
	zval *self = getThis(), *safe;
	int router_e_len, ret;
	char *router_e;
	safe = zend_read_property(gene_router_ce, self, GENE_ROUTER_SAFE,
			strlen(GENE_ROUTER_SAFE), 1 TSRMLS_CC);
	if (Z_STRLEN_P(safe)) {
		router_e_len = spprintf(&router_e, 0, "%s%s", Z_STRVAL_P(safe),
				GENE_ROUTER_ROUTER_EVENT);
	} else {
		router_e_len = spprintf(&router_e, 0, "%s", GENE_ROUTER_ROUTER_EVENT);
	}
	ret = gene_cache_del(router_e, router_e_len TSRMLS_CC);
	if (ret) {
		efree(router_e);
		RETURN_TRUE
		;
	}
	efree(router_e);
	RETURN_FALSE
	;
}
/* }}} */

/*
 * {{{ public gene_router::clear()
 */
PHP_METHOD(gene_router, clear) {
	zval *self = getThis(), *safe;
	int router_e_len, ret;
	char *router_e;
	safe = zend_read_property(gene_router_ce, self, GENE_ROUTER_SAFE,
			strlen(GENE_ROUTER_SAFE), 1 TSRMLS_CC);
	if (Z_STRLEN_P(safe)) {
		router_e_len = spprintf(&router_e, 0, "%s%s", Z_STRVAL_P(safe),
				GENE_ROUTER_ROUTER_TREE);
	} else {
		router_e_len = spprintf(&router_e, 0, "%s", GENE_ROUTER_ROUTER_TREE);
	}
	ret = gene_cache_del(router_e, router_e_len TSRMLS_CC);
	if (ret) {
		efree(router_e);
	}
	if (Z_STRLEN_P(safe)) {
		router_e_len = spprintf(&router_e, 0, "%s%s", Z_STRVAL_P(safe),
				GENE_ROUTER_ROUTER_EVENT);
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
PHP_METHOD(gene_router, getTime) {
	zval *self = getThis(), *safe;
	int router_e_len;
	long ctime = 0;
	char *router_e;
	safe = zend_read_property(gene_router_ce, self, GENE_ROUTER_SAFE,
			strlen(GENE_ROUTER_SAFE), 1 TSRMLS_CC);
	if (Z_STRLEN_P(safe)) {
		router_e_len = spprintf(&router_e, 0, "%s%s", Z_STRVAL_P(safe),
				GENE_ROUTER_ROUTER_TREE);
	} else {
		router_e_len = spprintf(&router_e, 0, "%s", GENE_ROUTER_ROUTER_TREE);
	}
	ctime = gene_cache_getTime(router_e, router_e_len TSRMLS_CC);
	efree(router_e);
	if (ctime > 0) {
		ctime = time(NULL) - ctime;
		RETURN_LONG(ctime);
	}
	RETURN_NULL()
	;
}
/* }}} */

/*
 * {{{ public gene_router::getRouter()
 */
PHP_METHOD(gene_router, getRouter) {
	zval *self = getThis();
	RETURN_ZVAL(self, 1, 0);
}
/* }}} */

/*
 * {{{ public gene_router::readFile()
 */
PHP_METHOD(gene_router, readFile) {
	char *fileName = NULL, *rec = NULL;
	int fileNameLen = 0;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &fileName,
			&fileNameLen) == FAILURE) {
		RETURN_NULL()
		;
	}
	rec = readfilecontent(fileName);
	if (rec != NULL) {
		RETURN_STRING(rec, 0);
	}
	RETURN_NULL()
	;
}
/* }}} */

static void php_free_pcre_cache(void *data) /* {{{ */
{
	pcre_cache_entry *pce = (pcre_cache_entry *) data;
	if (!pce)
		return;
	pefree(pce->re, 1);
	if (pce->extra)
		pefree(pce->extra, 1);
#if HAVE_SETLOCALE
	if ((void*) pce->tables)
		pefree((void* )pce->tables, 1);
	pefree(pce->locale, 1);
#endif
}
/* }}} */

/** {{{ public gene_router::display(string $file)
 */
PHP_METHOD(gene_router, display) {
	char *file;
	int file_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|l", &file,
			&file_len) == FAILURE) {
		return;
	}
	if (file_len) {
		gene_view_display(file TSRMLS_CC);
	}
}
/* }}} */

/** {{{ public gene_router::display(string $file)
 */
PHP_METHOD(gene_router, displayExt) {
	char *file, *parent_file = NULL;
	int file_len = 0, parent_file_len = 0;
	zend_bool isCompile = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|sb", &file,
			&file_len, &parent_file, &parent_file_len, &isCompile) == FAILURE) {
		return;
	}
	if (parent_file_len) {
		GENE_G(child_views) = estrndup(file, file_len);
		gene_view_display_ext(parent_file, isCompile TSRMLS_CC);
	} else {
		gene_view_display_ext(file, isCompile TSRMLS_CC);
	}
}
/* }}} */

/*
 * {{{ public gene_router::readFile()
 */
PHP_METHOD(gene_router, match) {
	char *fileName = NULL, *rec = NULL;
	int fileNameLen = 0;
	pcre_cache_entry *pce = NULL;
	zval *result_match, *match_long, *self = getThis();
	zval **data, *ret = NULL;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &fileName,
			&fileNameLen) == FAILURE) {
		RETURN_NULL()
		;
	}
	rec = readfilecontent(fileName);
	if (rec != NULL) {
		pce = pcre_get_compiled_regex_cache(GENE_ROUTER_CONTENT_REG,
				strlen(GENE_ROUTER_CONTENT_REG) TSRMLS_CC);
		if (pce != NULL) {
			MAKE_STD_ZVAL(match_long);
			MAKE_STD_ZVAL(result_match);
			array_init(result_match);
			php_pcre_match_impl(pce, rec, strlen(rec), match_long, result_match,
					1, 0, 0, 0 TSRMLS_CC);
			efree(rec);
			zval_ptr_dtor(&match_long);
			if (zend_hash_index_find(result_match->value.ht, 0,
					(void **) &data) == SUCCESS) {
				ret = gene_cache_zval_losable(*data TSRMLS_CC);
				zval_ptr_dtor(&result_match);
				RETURN_ZVAL(ret, 1, 1);
			}
		}
		efree(rec);
	}
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
	PHP_ME(gene_router, display, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(gene_router, displayExt, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(gene_router, runError, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(gene_router, run, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(gene_router, readFile, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(gene_router, match, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(gene_router, __call, gene_router_call_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(gene_router, __construct, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	{ NULL,NULL, NULL }
};
/* }}} */

/*
 * {{{ GENE_MINIT_FUNCTION
 */
GENE_MINIT_FUNCTION(router) {
	zend_class_entry gene_router;
	GENE_INIT_CLASS_ENTRY(gene_router, "Gene_Router", "Gene\\Router",
			gene_router_methods);
	gene_router_ce = zend_register_internal_class(&gene_router TSRMLS_CC);

	//prop
	zend_declare_property_string(gene_router_ce, GENE_ROUTER_SAFE,
			strlen(GENE_ROUTER_SAFE), "", ZEND_ACC_PUBLIC TSRMLS_CC);
	zend_declare_property_string(gene_router_ce, GENE_ROUTER_PREFIX,
			strlen(GENE_ROUTER_PREFIX), "", ZEND_ACC_PUBLIC TSRMLS_CC);
	//zend_declare_property_string(gene_router_ce, GENE_ROUTER_PREFIX, strlen(GENE_ROUTER_PREFIX), "", ZEND_ACC_PUBLIC TSRMLS_CC);
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
