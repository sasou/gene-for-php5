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
#include "gene_load.h"
#include "gene_config.h"
#include "gene_common.h"

zend_class_entry * gene_load_ce;

/** {{{ int gene_load_import(char *path TSRMLS_DC)
*/
int gene_load_import(char *path TSRMLS_DC) {
	zend_file_handle file_handle;
	zend_op_array 	*op_array;
	char realpath[MAXPATHLEN];

	if (!VCWD_REALPATH(path, realpath)) {
		return 0;
	}

	file_handle.filename = path;
	file_handle.free_filename = 0;
	file_handle.type = ZEND_HANDLE_FILENAME;
	file_handle.opened_path = NULL;
	file_handle.handle.fp = NULL;

	op_array = zend_compile_file(&file_handle, ZEND_INCLUDE TSRMLS_CC);

	if (op_array && file_handle.handle.stream.handle) {
		int dummy = 1;

		if (!file_handle.opened_path) {
			file_handle.opened_path = path;
		}

		zend_hash_add(&EG(included_files), file_handle.opened_path, strlen(file_handle.opened_path)+1, (void *)&dummy, sizeof(int), NULL);
	}
	zend_destroy_file_handle(&file_handle TSRMLS_CC);

	if (op_array) {
		zval *result = NULL;

		GENE_STORE_EG_ENVIRON();

		EG(return_value_ptr_ptr) = &result;
		EG(active_op_array) 	 = op_array;

#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION > 2)) || (PHP_MAJOR_VERSION > 5)
		if (!EG(active_symbol_table)) {
#if PHP_MINOR_VERSION < 5
			zval *orig_this = EG(This);
			EG(This) = NULL;
			zend_rebuild_symbol_table(TSRMLS_C);
			EG(This) = orig_this;
#else
			zend_rebuild_symbol_table(TSRMLS_C);
#endif

		}
#endif
		zend_execute(op_array TSRMLS_CC);

		destroy_op_array(op_array TSRMLS_CC);
		efree(op_array);
		if (!EG(exception)) {
			if (EG(return_value_ptr_ptr) && *EG(return_value_ptr_ptr)) {
				zval_ptr_dtor(EG(return_value_ptr_ptr));
			}
		}
		GENE_RESTORE_EG_ENVIRON();
	    return 1;
	}

	return 0;
}
/* }}} */

/** {{{ int gene_loader_register(zval *loader,char *methodName TSRMLS_DC)
*/
int gene_loader_register(zval *loader,char *methodName TSRMLS_DC) {
	zval *autoload, *method, *function, *ret = NULL;
	zval **params[1] = {&autoload};

	MAKE_STD_ZVAL(autoload);
	array_init(autoload);

	MAKE_STD_ZVAL(method);
	if (methodName) {
		ZVAL_STRING(method, methodName, 1);
	} else {
		ZVAL_STRING(method, GENE_AUTOLOAD_FUNC_NAME, 1);
	}

	zend_hash_next_index_insert(Z_ARRVAL_P(autoload), &loader, sizeof(zval *), NULL);
	zend_hash_next_index_insert(Z_ARRVAL_P(autoload), &method, sizeof(zval *), NULL);

	MAKE_STD_ZVAL(function);
	ZVAL_STRING(function, GENE_SPL_AUTOLOAD_REGISTER_NAME, 0);

	do {
		zend_fcall_info fci = {
			sizeof(fci),
			EG(function_table),
			function,
			NULL,
			&ret,
			1,
			(zval ***)params,
			NULL,
			1
		};

		if (zend_call_function(&fci, NULL TSRMLS_CC) == FAILURE) {
			if (ret) {
				zval_ptr_dtor(&ret);
			}
			efree(function);
			zval_ptr_dtor(&autoload);
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to register autoload function %s", GENE_AUTOLOAD_FUNC_NAME);
			return 0;
		}

		if (ret) {
			zval_ptr_dtor(&ret);
		}
		efree(function);
		zval_ptr_dtor(&autoload);
	} while (0);
	return 1;
}
/* }}} */

/** {{{ int gene_loader_register_function(char *methodName TSRMLS_DC)
*/
int gene_loader_register_function(TSRMLS_DC) {
	zval *method, *function, *ret = NULL;
	zval **params[1] = {&method};

	MAKE_STD_ZVAL(method);
	if (GENE_G(auto_load_fun)) {
		ZVAL_STRING(method, GENE_G(auto_load_fun), 1);
	} else {
		ZVAL_STRING(method, GENE_AUTOLOAD_FUNC_NAME, 1);
	}

	MAKE_STD_ZVAL(function);
	ZVAL_STRING(function, GENE_SPL_AUTOLOAD_REGISTER_NAME, 0);

	do {
		zend_fcall_info fci = {
			sizeof(fci),
			EG(function_table),
			function,
			NULL,
			&ret,
			1,
			(zval ***)params,
			NULL,
			1
		};

		if (zend_call_function(&fci, NULL TSRMLS_CC) == FAILURE) {
			if (ret) {
				zval_ptr_dtor(&ret);
			}
			efree(function);
			zval_ptr_dtor(&method);
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to register autoload function %s", GENE_G(auto_load_fun));
			return 0;
		}

		if (ret) {
			zval_ptr_dtor(&ret);
		}
		efree(function);
		zval_ptr_dtor(&method);
	} while (0);
	return 1;
}
/* }}} */

/*
 *  {{{ zval *gene_load_instance(zval *this_ptr TSRMLS_DC)
 */
zval *gene_load_instance(zval *this_ptr TSRMLS_DC)
{
	zval *instance = zend_read_static_property(gene_load_ce, GENE_LOAD_PROPERTY_INSTANCE, strlen(GENE_LOAD_PROPERTY_INSTANCE), 1 TSRMLS_CC);

	if (Z_TYPE_P(instance) == IS_OBJECT) {
		return instance;
	}
	if (this_ptr) {
		instance = this_ptr;
	} else {
		MAKE_STD_ZVAL(instance);
		object_init_ex(instance, gene_load_ce);
	}
	zend_update_static_property(gene_load_ce, GENE_LOAD_PROPERTY_INSTANCE, strlen(GENE_LOAD_PROPERTY_INSTANCE), instance TSRMLS_CC);
	return instance;
}
/* }}} */

/*
 * {{{ gene_load
 */
PHP_METHOD(gene_load, __construct)
{
	long debug = 0;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"|l", &debug) == FAILURE)
    {
        RETURN_NULL();
    }
}
/* }}} */

/*
 * {{{ public gene_load::autoload($key)
 */
PHP_METHOD(gene_load, autoload)
{
	int fileNmae_len = 0,filePath_len = 0;
	char *fileNmae = NULL,*filePath = NULL;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &fileNmae, &fileNmae_len) == FAILURE) {
		return;
	}
	replaceAll(fileNmae,'_','/');
	if (GENE_G(directory)) {
		filePath_len = spprintf(&filePath, 0, "%s%s.php", GENE_G(app_root), fileNmae);
	} else {
		filePath_len = spprintf(&filePath, 0, "%s.php", fileNmae);
	}
	gene_load_import(filePath TSRMLS_CC);
    efree(filePath);
    RETURN_TRUE;
}
/* }}} */

/*
 * {{{ public gene_load::load($key)
 */
PHP_METHOD(gene_load, import)
{
	zval *self = getThis();
	char *php_script;
	int php_script_len = 0;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|s", &php_script, &php_script_len) == FAILURE) {
		return;
	}
	if (php_script_len) {
		gene_load_import(php_script TSRMLS_CC);
	}
	RETURN_ZVAL(self, 1, 0);
}
/* }}} */

/*
 *  {{{ public gene_reg::getInstance(void)
 */
PHP_METHOD(gene_load, getInstance)
{
	zval *load = gene_load_instance(NULL TSRMLS_CC);
	RETURN_ZVAL(load, 1, 0);
}
/* }}} */


/*
 * {{{ gene_load_methods
 */
zend_function_entry gene_load_methods[] = {
		PHP_ME(gene_load, getInstance, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
		PHP_ME(gene_load, import, NULL, ZEND_ACC_PUBLIC)
		PHP_ME(gene_load, autoload, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
		PHP_ME(gene_load, __construct, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
		{NULL, NULL, NULL}
};
/* }}} */


/*
 * {{{ GENE_MINIT_FUNCTION
 */
GENE_MINIT_FUNCTION(load)
{
    zend_class_entry gene_load;
    INIT_CLASS_ENTRY(gene_load,"gene_load",gene_load_methods);
    gene_load_ce = zend_register_internal_class(&gene_load TSRMLS_CC);

	//static
    zend_declare_property_null(gene_load_ce, GENE_LOAD_PROPERTY_INSTANCE, strlen(GENE_LOAD_PROPERTY_INSTANCE),  ZEND_ACC_PROTECTED|ZEND_ACC_STATIC TSRMLS_CC);

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
