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
#include "zend_exceptions.h"

#include "php_gene.h"
#include "gene_application.h"
#include "gene_cache.h"
#include "gene_common.h"

zend_class_entry * gene_cache_ce;

static zval * gene_cache_zval_persistent(zval *zvalue TSRMLS_DC);
static void gene_cache_copy_persistent(HashTable *pdst,
		HashTable *src TSRMLS_DC);
static void gene_cache_copy_losable(HashTable *pdst, HashTable *src TSRMLS_DC);

char * str_add(const char *s, int length) {
	char *p;
#ifdef ZEND_SIGNALS
	TSRMLS_FETCH();
#endif

	HANDLE_BLOCK_INTERRUPTIONS();

	p = (char *) calloc(length + 1, sizeof(char));
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

/** {{{ static void gene_cache_dtor(gene_cache_container **cache)
 */
char * r_strndup(void *ptr, const char *s, int length) {
	char *p;
#ifdef ZEND_SIGNALS
	TSRMLS_FETCH();
#endif

	HANDLE_BLOCK_INTERRUPTIONS();

	p = (char *) realloc(ptr, length + 1);
	memset(p, 0, length + 1);
	if (UNEXPECTED(p == NULL)) {
		HANDLE_UNBLOCK_INTERRUPTIONS();
		return p;
	}
	if (length) {
		memcpy(p, s, length);
	}
	p[length] = 0;
	HANDLE_UNBLOCK_INTERRUPTIONS();
	//php_printf("realloc P:%p;",p);
	return p;
}

/* }}} */

/** {{{ static void gene_cache_zval_dtor(zval **value)
 */
static void gene_cache_zval_dtor(zval **zvalue) {
	if (*zvalue) {
		switch (Z_TYPE_PP(zvalue)) {
		case IS_STRING:
		case IS_CONSTANT:
			CHECK_ZVAL_STRING(*zvalue);
			//php_printf("array child P:%p;",(*zvalue)->value.str.val);
			if ((*zvalue)->value.str.val != NULL) {
				pefree((*zvalue)->value.str.val, 1);
				(*zvalue)->value.str.val = NULL;
			}
			break;
#ifdef IS_CONSTANT_ARRAY
		case IS_CONSTANT_ARRAY:
#endif
		case IS_ARRAY: {
			if ((*zvalue)->value.ht != NULL) {
				//php_printf(" destroy:%p start; ",(*zvalue)->value.ht);
				zend_hash_destroy((*zvalue)->value.ht);
				pefree((*zvalue)->value.ht, 1);
				(*zvalue)->value.ht = NULL;
			}
		}
			break;
		}
		pefree(*zvalue, 1);
		*zvalue = NULL;
	}
}
/* }}} */

/** {{{ static void gene_cache_dtor(gene_cache_container **cache)
 */
static void gene_cache_dtor(gene_cache_container **cache TSRMLS_DC) {
	if (*cache) {
		if ((*cache)->data) {
			switch (Z_TYPE_P((*cache)->data)) {
			case IS_STRING:
			case IS_CONSTANT:
				CHECK_ZVAL_STRING((*cache)->data);
				pefree((*cache)->data->value.str.val, 1);
				(*cache)->data->value.str.val = NULL;
				break;
#ifdef IS_CONSTANT_ARRAY
			case IS_CONSTANT_ARRAY:
#endif
			case IS_ARRAY: {
				zend_hash_destroy((*cache)->data->value.ht);
				pefree((*cache)->data->value.ht, 1);
				(*cache)->data->value.ht = NULL;
			}
				break;
			}
			pefree((*cache)->data, 1);
			(*cache)->data = NULL;
		}
		pefree(*cache, 1);
		*cache = NULL;
	}
}
/* }}} */

/** {{{ static void gene_cache_easy_dtor(gene_cache_container_easy **cache TSRMLS_DC)
 */
static void gene_cache_easy_dtor(gene_cache_container_easy **cache TSRMLS_DC) {
	if (*cache) {
		pefree(*cache, 1);
		*cache = NULL;
	}
}
/* }}} */

/*
 * {{{ static void * gene_cache_init(TSRMLS_D)
 */
void gene_cache_init(TSRMLS_D) {
	if (!GENE_G(cache)) {
		GENE_G(cache) = (HashTable *) pemalloc(sizeof(HashTable), 1);
		if (!GENE_G(cache)) {
			return;
		}
		zend_hash_init(GENE_G(cache), 10, NULL, (dtor_func_t ) gene_cache_dtor,
				1);
	}
	if (!GENE_G(cache_easy)) {
		GENE_G(cache_easy) = (HashTable *) pemalloc(sizeof(HashTable), 1);
		if (!GENE_G(cache_easy)) {
			return;
		}
		zend_hash_init(GENE_G(cache_easy), 10, NULL,
				(dtor_func_t ) gene_cache_easy_dtor, 1);
	}
	return;
}
/* }}} */

/** {{{ static zval * gene_cache_zval_persistent(zval *zvalue TSRMLS_DC)
 */
static zval * gene_cache_zval_persistent(zval *zvalue TSRMLS_DC) {
	HashTable *tmp_ht;
	zval *ret = (zval *) pemalloc(sizeof(zval), 1);
	INIT_PZVAL(ret);
	if (zvalue) {
		GENE_CZAL(ret, zvalue);
		switch (ret->type) {
		case IS_OBJECT:
			break;
		case IS_BOOL:
			ret->value.lval = zvalue->value.lval;
			Z_TYPE_P(ret) = IS_BOOL;
			break;
		case IS_LONG:
			ret->value.lval = zvalue->value.lval;
			Z_TYPE_P(ret) = IS_LONG;
			break;
		case IS_DOUBLE:
			ret->value.dval = zvalue->value.dval;
			Z_TYPE_P(ret) = IS_DOUBLE;
			break;
		case IS_NULL:
			break;
		case IS_CONSTANT:
		case IS_STRING:
			CHECK_ZVAL_STRING(zvalue);
			ret->value.str.len = zvalue->value.str.len;
			ret->value.str.val = str_add(zvalue->value.str.val,
					zvalue->value.str.len);
			break;
#ifdef IS_CONSTANT_ARRAY
		case IS_CONSTANT_ARRAY:
#endif
		case IS_ARRAY: {
			HashTable *tmp_ht, *original_ht = zvalue->value.ht;
			tmp_ht = (HashTable *) pemalloc(sizeof(HashTable), 1);
			if (!tmp_ht) {
				return NULL;
			}
			zend_hash_init(tmp_ht, zend_hash_num_elements(original_ht), NULL,
					(dtor_func_t ) gene_cache_zval_dtor, 1);
			gene_cache_copy_persistent(tmp_ht, original_ht TSRMLS_CC);
			ret->value.ht = tmp_ht;
		}
			break;
		}
	} else {
		Z_TYPE_P(ret) = IS_ARRAY;
		tmp_ht = (HashTable *) pemalloc(sizeof(HashTable), 1);
		if (!tmp_ht) {
			return NULL;
		}
		zend_hash_init(tmp_ht, 0, NULL, (dtor_func_t ) gene_cache_zval_dtor, 1);
		ret->value.ht = tmp_ht;
	}
	return ret;
}
/* }}} */

/** {{{ static void * gene_cache_zval_edit_persistent(zval *zvalue TSRMLS_DC)
 */
static void * gene_cache_zval_edit_persistent(zval *copyval,
		zval *zvalue TSRMLS_DC) {
	if (!zvalue)
		return NULL;
	switch (Z_TYPE_P(copyval)) {
	case IS_STRING:
		if ((Z_TYPE_P(zvalue) != IS_STRING)
				&& (copyval->value.str.val != NULL)) {
			pefree(copyval->value.str.val, 1);
			copyval->value.str.val = NULL;
		}
		break;
	case IS_ARRAY:
		if ((Z_TYPE_P(zvalue) != IS_ARRAY) && (copyval->value.ht != NULL)) {
			zend_hash_destroy(copyval->value.ht);
			pefree(copyval->value.ht, 1);
			copyval->value.ht = NULL;
		}
		break;
	}

	switch (Z_TYPE_P(zvalue)) {
	case IS_OBJECT:
		break;
	case IS_BOOL:
		copyval->value.lval = zvalue->value.lval;
		Z_TYPE_P(copyval) = IS_BOOL;
		break;
	case IS_LONG:
		copyval->value.lval = zvalue->value.lval;
		Z_TYPE_P(copyval) = IS_LONG;
		break;
	case IS_DOUBLE:
		copyval->value.dval = zvalue->value.dval;
		Z_TYPE_P(copyval) = IS_DOUBLE;
		break;
	case IS_NULL:
		break;
	case IS_CONSTANT:
	case IS_STRING:
		CHECK_ZVAL_STRING(zvalue);
		if (Z_TYPE_P(copyval) != IS_STRING) {
			copyval->value.str.val = str_add(zvalue->value.str.val,
					zvalue->value.str.len);
		} else {
			copyval->value.str.val = r_strndup(copyval->value.str.val,
					zvalue->value.str.val, zvalue->value.str.len);
		}
		Z_TYPE_P(copyval) = IS_STRING;
		copyval->value.str.len = zvalue->value.str.len;
		break;
#ifdef IS_CONSTANT_ARRAY
	case IS_CONSTANT_ARRAY:
#endif
	case IS_ARRAY: {
		if (Z_TYPE_P(copyval) != IS_ARRAY) {
			HashTable *tmp_ht;
			tmp_ht = (HashTable *) pemalloc(sizeof(HashTable), 1);
			if (!tmp_ht) {
				return NULL;
			}
			zend_hash_init(tmp_ht, zend_hash_num_elements(zvalue->value.ht),
					NULL, (dtor_func_t ) gene_cache_zval_dtor, 1);
			gene_cache_copy_persistent(tmp_ht, zvalue->value.ht TSRMLS_CC);
			copyval->value.ht = tmp_ht;
		} else {
			gene_cache_copy_persistent(copyval->value.ht,
					zvalue->value.ht TSRMLS_CC);
		}
		Z_TYPE_P(copyval) = IS_ARRAY;
	}
		break;
	}
	return NULL;
}
/* }}} */

/** {{{ zval * gene_cache_zval_losable(zval *zvalue TSRMLS_DC)
 */
zval * gene_cache_zval_losable(zval *zvalue TSRMLS_DC) {
	zval *ret;
	MAKE_STD_ZVAL(ret);
	switch (zvalue->type) {
	case IS_RESOURCE:
	case IS_OBJECT:
		break;
	case IS_BOOL:
		ret->value.lval = zvalue->value.lval;
		Z_TYPE_P(ret) = IS_BOOL;
		break;
	case IS_LONG:
		ret->value.lval = zvalue->value.lval;
		Z_TYPE_P(ret) = IS_LONG;
		break;
	case IS_DOUBLE:
		ret->value.dval = zvalue->value.dval;
		Z_TYPE_P(ret) = IS_DOUBLE;
		break;
	case IS_NULL:
		break;
	case IS_CONSTANT:
	case IS_STRING:
		CHECK_ZVAL_STRING(zvalue);
		ZVAL_STRINGL(ret, zvalue->value.str.val, zvalue->value.str.len, 1);
		break;
#ifdef IS_CONSTANT_ARRAY
	case IS_CONSTANT_ARRAY:
#endif
	case IS_ARRAY: {
		HashTable *original_ht = zvalue->value.ht;
		array_init(ret);
		gene_cache_copy_losable(ret->value.ht, original_ht TSRMLS_CC);
	}
		break;
	}

	return ret;
}
/* }}} */

/** {{{ static void gene_cache_copy_losable(HashTable *ldst, HashTable *src TSRMLS_DC)
 */
static void gene_cache_copy_losable(HashTable *ldst, HashTable *src TSRMLS_DC) {
	zval **ppzval, *tmp;
	char *key;
	long idx;
	int keylen;

	for (zend_hash_internal_pointer_reset(src);
			zend_hash_has_more_elements(src) == SUCCESS;
			zend_hash_move_forward(src)) {
		if (zend_hash_get_current_key_ex(src, &key, &keylen, &idx, 0,
				NULL) == HASH_KEY_IS_LONG) {
			if (zend_hash_get_current_data(src, (void**)&ppzval) == FAILURE) {
				continue;
			}
			tmp = gene_cache_zval_losable(*ppzval TSRMLS_CC);
			zend_hash_index_update(ldst, idx, (void ** )&tmp, sizeof(zval *),
					NULL);

		} else {
			if (zend_hash_get_current_data(src, (void**)&ppzval) == FAILURE) {
				continue;
			}
			tmp = gene_cache_zval_losable(*ppzval TSRMLS_CC);
			zend_hash_update(ldst, key, keylen, (void ** )&tmp, sizeof(zval *),
					NULL);
		}
	}
}
/* }}} */

/** {{{ static void gene_cache_copy_persistent(HashTable *pdst, HashTable *src TSRMLS_DC)
 */
static void gene_cache_copy_persistent(HashTable *pdst,
		HashTable *src TSRMLS_DC) {
	zval **ppzval;
	char *key, *keyval;
	int keylen, keyvallen;
	long idx;
	for (zend_hash_internal_pointer_reset(pdst);
			zend_hash_has_more_elements(pdst) == SUCCESS;
			zend_hash_move_forward(pdst)) {
		if (zend_hash_get_current_key_ex(pdst, &key, &keylen, &idx, 0,
				NULL) == HASH_KEY_IS_LONG) {
			if (zend_hash_index_exists(src, idx) != 1) {
				zend_hash_index_del(pdst, idx);
			}

		} else {
			if (zend_hash_exists(src, key, keylen) != 1) {
				zend_hash_del(pdst, key, keylen);
			}
		}
	}
	for (zend_hash_internal_pointer_reset(src);
			zend_hash_has_more_elements(src) == SUCCESS;
			zend_hash_move_forward(src)) {
		if (zend_hash_get_current_key_ex(src, &key, &keylen, &idx, 0,
				NULL) == HASH_KEY_IS_LONG) {
			zval **copyval, *tmp;
			if (zend_hash_get_current_data(src, (void**)&ppzval) == FAILURE) {
				continue;
			}
			if (zend_hash_index_find(pdst, idx, (void**) &copyval) == FAILURE) {
				tmp = gene_cache_zval_persistent(*ppzval TSRMLS_CC);
				if (tmp) {
					zend_hash_index_update(pdst, idx, (void ** )&tmp,
							sizeof(zval *), NULL);
				}
			} else {
				gene_cache_zval_edit_persistent(*copyval, *ppzval TSRMLS_CC);
			}

		} else {
			zval **copyval, *tmp;
			if (zend_hash_get_current_data(src, (void**)&ppzval) == FAILURE) {
				continue;
			}
			if (zend_hash_find(pdst, key, keylen + 1,
					(void**) &copyval) == FAILURE) {
				tmp = gene_cache_zval_persistent(*ppzval TSRMLS_CC);
				if (tmp) {
					keyvallen = spprintf(&keyval, 0, "%s", key);
					zend_hash_update(pdst, keyval, keyvallen + 1,
							(void ** )&tmp, sizeof(zval *), NULL);
					efree(keyval);
					keyval = NULL;
				}
			} else {
				gene_cache_zval_edit_persistent(*copyval, *ppzval TSRMLS_CC);
			}
		}
	}
}
/* }}} */

/** {{{ void gene_cache_set(char *keyString,int keyString_len,zval *zvalue, int validity TSRMLS_DC)
 */
void gene_cache_set(char *keyString, int keyString_len, zval *zvalue,
		int validity TSRMLS_DC) {
	char *key;
	int len;
	zval *tmp;
	gene_cache_container **copyval, *ret;
	if (zvalue) {
		if (zend_hash_find(GENE_G(cache), keyString, keyString_len + 1,
				(void **) &copyval) == FAILURE) {
			ret = (gene_cache_container *) pemalloc(
					sizeof(gene_cache_container), 1);
			tmp = gene_cache_zval_persistent(zvalue TSRMLS_CC);
			ret->ctime = time(NULL);
			ret->validity = validity;
			ret->status = 0;
			ret->data = tmp;
			len = spprintf(&key, 0, "%s", keyString);
			zend_hash_add(GENE_G(cache), key, len + 1, &ret,
					sizeof(gene_cache_container *), NULL);
			efree(key);
			key = NULL;
			return;
		}
		(*copyval)->ctime = time(NULL);
		(*copyval)->validity = validity;
		if ((*copyval)->status == 0) {
			(*copyval)->status = 1;
			gene_cache_zval_edit_persistent((*copyval)->data, zvalue TSRMLS_CC);
			(*copyval)->status = 0;
		}
	}
}
/* }}} */

/** {{{ void gene_cache_get(char *keyString, int keyString_len TSRMLS_DC)
 */
zval * gene_cache_get(char *keyString, int keyString_len TSRMLS_DC) {
	int ctime, validity;
	zval *ret;
	gene_cache_container **zvalue;
	if (zend_hash_find(GENE_G(cache), keyString, keyString_len + 1,
			(void **) &zvalue) == SUCCESS) {
		validity = (*zvalue)->validity;
		if (validity > 0) {
			ctime = (*zvalue)->ctime + validity;
			if (time(NULL) > ctime) {
				zend_hash_del(GENE_G(cache), keyString, keyString_len + 1);
				return NULL;
			}
		}
		if ((*zvalue)->status == 0) {
			(*zvalue)->status = 1;
			ret = gene_cache_zval_losable((*zvalue)->data TSRMLS_CC);
			(*zvalue)->status = 0;
			return ret;
		}
		return NULL;
	}
	return NULL;
}
/* }}} */

/** {{{ void gene_cache_get_quick(char *keyString, int keyString_len TSRMLS_DC)
 */
zval * gene_cache_get_quick(char *keyString, int keyString_len TSRMLS_DC) {
	gene_cache_container **zvalue;
	if (zend_hash_find(GENE_G(cache), keyString, keyString_len + 1,
			(void **) &zvalue) == SUCCESS) {
		return (*zvalue)->data;
	}
	return NULL;
}
/* }}} */


/** {{{ zval * gene_cache_get_by_config(char *keyString, int keyString_len,char *path TSRMLS_DC)
 */
zval * gene_cache_get_by_config(char *keyString, int keyString_len,
		char *path TSRMLS_DC) {
	char *ptr = NULL, *seg = NULL;
	zval *tmp = NULL;
	zval **ret = NULL;
	gene_cache_container **copyval = NULL;
	if (zend_hash_find(GENE_G(cache), keyString, keyString_len + 1,
			(void **) &copyval) == SUCCESS) {
		tmp = (*copyval)->data;
		if (path != NULL) {
			replaceAll(path, '.', '/');
			seg = php_strtok_r(path, "/", &ptr);
			while (seg) {
				if (zend_hash_find(Z_ARRVAL_P(tmp), seg, strlen(seg) + 1,
						(void **) &ret) == FAILURE) {
					tmp = NULL;
					return NULL;
				}
				tmp = (*ret);
				seg = php_strtok_r(NULL, "/", &ptr);
			}
			if (ret) {
				tmp = NULL;
				return gene_cache_zval_losable(*ret TSRMLS_CC);
			}
			return NULL;
		}
		return gene_cache_zval_losable(tmp TSRMLS_CC);

	}
	tmp = NULL;
	return NULL;
}
/* }}} */

/** {{{  void gene_cache_set_easy(char *keyString, int keyString_len, int time, int validity TSRMLS_DC)
 */
void gene_cache_set_easy(char *keyString, int keyString_len, int times,
		int validity TSRMLS_DC) {
	char *key;
	int len;
	gene_cache_container_easy **val, *ret;
	if (zend_hash_find(GENE_G(cache_easy), keyString, keyString_len + 1,
			(void **) &val) == FAILURE) {
		ret = (gene_cache_container_easy *) pemalloc(
				sizeof(gene_cache_container_easy), 1);
		ret->stime = time(NULL);
		ret->validity = validity;
		ret->status = 0;
		ret->ftime = times;
		len = spprintf(&key, 0, "%s", keyString);
		zend_hash_add(GENE_G(cache_easy), keyString, keyString_len + 1, &ret,
				sizeof(gene_cache_container_easy *), NULL);
		efree(key);
		key = NULL;
		return;
	}
	(*val)->stime = time(NULL);
	(*val)->validity = validity;
	(*val)->status = 0;
	(*val)->ftime = times;
	return;
}
/* }}} */

/** {{{ void gene_cache_get_easy(char *keyString, int keyString_len TSRMLS_DC)
 */
gene_cache_container_easy * gene_cache_get_easy(char *keyString,
		int keyString_len TSRMLS_DC) {
	gene_cache_container_easy **zvalue;
	if (zend_hash_find(GENE_G(cache_easy), keyString, keyString_len + 1,
			(void **) &zvalue) == SUCCESS) {
		return (*zvalue);
	}
	return NULL;
}
/* }}} */

/** {{{ void gene_cache_set_by_router(char *keyString, int keyString_len, char *path, zval *zvalue, int validity TSRMLS_DC)
 */
void gene_cache_set_by_router(char *keyString, int keyString_len, char *path,
		zval *zvalue, int validity TSRMLS_DC) {
	char *key, *ptr = NULL, *seg = NULL;
	int len;
	zval *tmp, *val;
	gene_cache_container **copyval = NULL, *ret = NULL;
	if (zend_hash_find(GENE_G(cache), keyString, keyString_len + 1,
			(void **) &copyval) == FAILURE) {
		ret = (gene_cache_container *) pemalloc(sizeof(gene_cache_container),
				1);
		tmp = gene_cache_zval_persistent(NULL TSRMLS_CC);
		ret->ctime = time(NULL);
		ret->validity = validity;
		ret->data = tmp;
		ret->status = 0;
		len = spprintf(&key, 0, "%s", keyString);
		zend_hash_add(GENE_G(cache), key, len + 1, &ret,
				sizeof(gene_cache_container *), NULL);
		efree(key);
		seg = php_strtok_r(path, "/", &ptr);
		while (seg) {
			if (ptr && strlen(ptr) > 0) {
				val = gene_cache_set_val(tmp, seg, strlen(seg), NULL TSRMLS_CC);
			} else {
				val = gene_cache_set_val(tmp, seg, strlen(seg),
						zvalue TSRMLS_CC);
			}
			tmp = val;
			seg = php_strtok_r(NULL, "/", &ptr);
		}
	} else {
		(*copyval)->ctime = time(NULL);
		(*copyval)->validity = validity;
		if ((*copyval)->status == 0) {
			(*copyval)->status = 1;
			tmp = (*copyval)->data;
			seg = php_strtok_r(path, "/", &ptr);
			while (seg) {
				if (ptr && strlen(ptr) > 0) {
					tmp = gene_cache_set_val(tmp, seg, strlen(seg),
							NULL TSRMLS_CC);
				} else {
					tmp = gene_cache_set_val(tmp, seg, strlen(seg),
							zvalue TSRMLS_CC);
				}
				seg = php_strtok_r(NULL, "/", &ptr);
			}
			(*copyval)->status = 0;
		}
		return;

	}
	return;
}
/* }}} */

/** {{{ static void gene_cache_exists(char *keyString, int keyString_len TSRMLS_DC)
 */
zval* gene_cache_set_val(zval *val, char *keyString, int keyString_len,
		zval *zvalue TSRMLS_DC) {
	char *key;
	int len;
	zval *tmp, **copyval;
	if (zend_hash_find(Z_ARRVAL_P(val), keyString, keyString_len + 1,
			(void **) &copyval) == FAILURE) {
		if (zvalue) {
			tmp = gene_cache_zval_persistent(zvalue TSRMLS_CC);
		} else {
			tmp = gene_cache_zval_persistent(NULL TSRMLS_CC);
		}
		len = spprintf(&key, 0, "%s", keyString);
		zend_hash_add(Z_ARRVAL_P(val), key, len + 1, (void ** )&tmp,
				sizeof(zval *), NULL);
		efree(key);
	} else {
		if (zvalue) {
			gene_cache_zval_edit_persistent(*copyval, zvalue TSRMLS_CC);
		}
		return *copyval;
	}
	return tmp;
}
/* }}} */

/** {{{ void gene_cache_getTime(char *keyString, int keyString_len,gene_cache_container **zvalue TSRMLS_DC)
 */
long gene_cache_getTime(char *keyString, int keyString_len TSRMLS_DC) {
	gene_cache_container **zvalue;
	if (zend_hash_find(GENE_G(cache), keyString, keyString_len + 1,
			(void **) &zvalue) == FAILURE) {
		return 0;
	}
	return (*zvalue)->ctime;
}
/* }}} */

/** {{{ void gene_cache_exists(char *keyString, int keyString_len TSRMLS_DC)
 */
int gene_cache_exists(char *keyString, int keyString_len TSRMLS_DC) {
	if (zend_hash_exists(GENE_G(cache), keyString, keyString_len + 1) != 1) {
		return 0;
	}
	return 1;
}
/* }}} */

/** {{{ void gene_cache_del(char *keyString, int keyString_len TSRMLS_DC)
 */
int gene_cache_del(char *keyString, int keyString_len TSRMLS_DC) {
	if (zend_hash_del(GENE_G(cache), keyString, keyString_len+1) == 0) {
		return 1;
	}
	return 0;
}
/* }}} */

/*
 * {{{ public gene_cache::__construct()
 */
PHP_METHOD(gene_cache, __construct) {
	zval *safe = NULL;
	int len = 0;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|z", &safe) == FAILURE) {
		RETURN_NULL()
		;
	}
	if (safe) {
		zend_update_property_string(gene_cache_ce, getThis(), GENE_CACHE_SAFE,
				strlen(GENE_CACHE_SAFE), Z_STRVAL_P(safe) TSRMLS_CC);
	} else {
		if (GENE_G(app_key)) {
			zend_update_property_string(gene_cache_ce, getThis(),
					GENE_CACHE_SAFE, strlen(GENE_CACHE_SAFE),
					GENE_G(app_key) TSRMLS_CC);
		} else {
			gene_ini_router(TSRMLS_C);
			zend_update_property_string(gene_cache_ce, getThis(),
					GENE_CACHE_SAFE, strlen(GENE_CACHE_SAFE),
					GENE_G(directory) TSRMLS_CC);
		}
	}
}
/* }}} */

/*
 * {{{ public gene_cache::set($key, $data)
 */
PHP_METHOD(gene_cache, set) {
	char *keyString, *router_e;
	int keyString_len, validity = 0, router_e_len;
	zval *zvalue, *safe;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz|l", &keyString,
			&keyString_len, &zvalue, &validity) == FAILURE) {
		return;
	}
	safe = zend_read_property(gene_cache_ce, getThis(), GENE_CACHE_SAFE,
			strlen(GENE_CACHE_SAFE), 1 TSRMLS_CC);
	if (Z_STRLEN_P(safe)) {
		router_e_len = spprintf(&router_e, 0, "%s:%s", Z_STRVAL_P(safe),
				keyString);
	} else {
		router_e_len = spprintf(&router_e, 0, ":%s", keyString);
	}
	if (zvalue) {
		gene_cache_set(router_e, router_e_len, zvalue, validity TSRMLS_CC);
		zval_ptr_dtor(&zvalue);
	}
	efree(router_e);
	RETURN_BOOL(1);
}
/* }}} */

/*
 * {{{ public gene_cache::get($key)
 */
PHP_METHOD(gene_cache, get) {
	char *keyString, *router_e;
	int keyString_len, router_e_len;
	zval *zvalue, *safe;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &keyString,
			&keyString_len) == FAILURE) {
		return;
	}
	safe = zend_read_property(gene_cache_ce, getThis(), GENE_CACHE_SAFE,
			strlen(GENE_CACHE_SAFE), 1 TSRMLS_CC);
	if (Z_STRLEN_P(safe)) {
		router_e_len = spprintf(&router_e, 0, "%s:%s", Z_STRVAL_P(safe),
				keyString);
	} else {
		router_e_len = spprintf(&router_e, 0, ":%s", keyString);
	}
	zvalue = gene_cache_get(router_e, router_e_len TSRMLS_CC);
	efree(router_e);
	if (zvalue) {
		RETURN_ZVAL(zvalue, 1, 1);
	}
	RETURN_NULL()
	;
}
/* }}} */

/*
 * {{{ public gene_cache::getTime($key)
 */
PHP_METHOD(gene_cache, getTime) {
	char *keyString, *router_e;
	int keyString_len, router_e_len, ret;
	zval *safe;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &keyString,
			&keyString_len) == FAILURE) {
		return;
	}
	safe = zend_read_property(gene_cache_ce, getThis(), GENE_CACHE_SAFE,
			strlen(GENE_CACHE_SAFE), 1 TSRMLS_CC);
	if (Z_STRLEN_P(safe)) {
		router_e_len = spprintf(&router_e, 0, "%s:%s", Z_STRVAL_P(safe),
				keyString);
	} else {
		router_e_len = spprintf(&router_e, 0, ":%s", keyString);
	}
	ret = gene_cache_getTime(router_e, router_e_len TSRMLS_CC);
	efree(router_e);
	RETURN_LONG(ret);
}
/* }}} */

/*
 * {{{ public gene_cache::exists($key)
 */
PHP_METHOD(gene_cache, exists) {
	char *keyString, *router_e;
	int keyString_len, router_e_len, ret;
	zval *safe;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &keyString,
			&keyString_len) == FAILURE) {
		return;
	}
	safe = zend_read_property(gene_cache_ce, getThis(), GENE_CACHE_SAFE,
			strlen(GENE_CACHE_SAFE), 1 TSRMLS_CC);
	if (Z_STRLEN_P(safe)) {
		router_e_len = spprintf(&router_e, 0, "%s:%s", Z_STRVAL_P(safe),
				keyString);
	} else {
		router_e_len = spprintf(&router_e, 0, ":%s", keyString);
	}
	ret = gene_cache_exists(router_e, router_e_len TSRMLS_CC);
	efree(router_e);
	RETURN_BOOL(ret);
}
/* }}} */

/*
 * {{{ public gene_cache::del($key)
 */
PHP_METHOD(gene_cache, del) {
	char *keyString, *router_e;
	int keyString_len, router_e_len, ret;
	zval *safe;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &keyString,
			&keyString_len) == FAILURE) {
		return;
	}
	safe = zend_read_property(gene_cache_ce, getThis(), GENE_CACHE_SAFE,
			strlen(GENE_CACHE_SAFE), 1 TSRMLS_CC);
	if (Z_STRLEN_P(safe)) {
		router_e_len = spprintf(&router_e, 0, "%s:%s", Z_STRVAL_P(safe),
				keyString);
	} else {
		router_e_len = spprintf(&router_e, 0, ":%s", keyString);
	}
	ret = gene_cache_del(router_e, router_e_len TSRMLS_CC);
	efree(router_e);
	RETURN_BOOL(ret);
}
/* }}} */

/*
 * {{{ gene_cache_methods
 */
zend_function_entry gene_cache_methods[] = {
	PHP_ME(gene_cache, set, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(gene_cache, get, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(gene_cache, getTime, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(gene_cache, exists, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(gene_cache, del, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(gene_cache, __construct, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	{ NULL, NULL, NULL }
};
/* }}} */

/*
 * {{{ GENE_MINIT_FUNCTION
 */
GENE_MINIT_FUNCTION(cache) {
	zend_class_entry gene_cache;
	GENE_INIT_CLASS_ENTRY(gene_cache, "Gene_Cache", "Gene\\Cache",
			gene_cache_methods);
	gene_cache_ce = zend_register_internal_class(&gene_cache TSRMLS_CC);

	//debug
	zend_declare_property_string(gene_cache_ce, GENE_CACHE_SAFE,
			strlen(GENE_CACHE_SAFE), "", ZEND_ACC_PUBLIC TSRMLS_CC);
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
