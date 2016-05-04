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
#include "gene_execute.h"

zend_class_entry * gene_execute_ce;

/*
 * {{{ void * gene_execite_opcodes_run(zend_op_array *op_array TSRMLS_DC)
 */
void *gene_execite_opcodes_run(zend_op_array *op_array TSRMLS_DC){
	zend_op_array *orig_op_array = EG(active_op_array);
	EG(active_op_array) = op_array;
	if (EG(active_op_array)) {
		zend_execute(EG(active_op_array) TSRMLS_CC);
		zend_exception_restore(TSRMLS_C);
		if (EG(exception)) {
			if (EG(user_exception_handler)) {
				zval *orig_user_exception_handler;
				zval **params[1], *retval2, *old_exception;
				old_exception = EG(exception);
				EG(exception) = NULL;
				params[0] = &old_exception;
				orig_user_exception_handler = EG(user_exception_handler);
				if (call_user_function_ex(CG(function_table), NULL, orig_user_exception_handler, &retval2, 1, params, 1, NULL TSRMLS_CC) == SUCCESS) {
					if (retval2 != NULL) {
						zval_ptr_dtor(&retval2);
					}
					if (EG(exception)) {
						zval_ptr_dtor(&EG(exception));
						EG(exception) = NULL;
					}
					zval_ptr_dtor(&old_exception);
				} else {
					EG(exception) = old_exception;
					zend_exception_error(EG(exception), E_ERROR TSRMLS_CC);
				}
			} else {
				zend_exception_error(EG(exception), E_ERROR TSRMLS_CC);
			}
		}
		destroy_op_array(EG(active_op_array) TSRMLS_CC);
		efree(EG(active_op_array));
	}
	EG(active_op_array) = orig_op_array;
	efree(orig_op_array);
	return NULL;
}
/* }}} */

/*
 * {{{ gene_execute_methods
 */
PHP_METHOD(gene_execute, __construct)
{
	long debug = 0;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"|l", &debug) == FAILURE)
    {
        RETURN_NULL();
    }
    if(debug) {
    	zend_update_property_long(gene_execute_ce, getThis(), GENE_EXECUTE_DEBUG, strlen(GENE_EXECUTE_DEBUG), debug TSRMLS_CC);
    } else {
    	zend_update_property_long(gene_execute_ce, getThis(), GENE_EXECUTE_DEBUG, strlen(GENE_EXECUTE_DEBUG), 0 TSRMLS_CC);
    }
}
/* }}} */

/*
 * {{{ public gene_execute::GetOpcodes($codeString)
 */
PHP_METHOD(gene_execute, GetOpcodes)
{
	char *php_script;
	int php_script_len, i;
	zval *zv, *opcodes_array,*debug;
	zend_op_array *op_array;
	debug = NULL;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &php_script, &php_script_len) == FAILURE) {
		return;
	}
	debug = zend_read_property(gene_execute_ce, getThis(), GENE_EXECUTE_DEBUG, strlen(GENE_EXECUTE_DEBUG), 0 TSRMLS_CC);
	if(Z_BVAL_P(debug)){
		php_printf(php_script);
	}
	MAKE_STD_ZVAL(zv);
	ZVAL_STRINGL(zv, php_script, php_script_len, 0);
	MAKE_STD_ZVAL(opcodes_array);
	array_init(opcodes_array);

	op_array = zend_compile_string(zv, "" TSRMLS_CC);
	if (! op_array) {
		return;
	}
	for (i = 0; i < op_array->last; i++) {
		zend_op op = op_array->opcodes[i];
		add_index_long(opcodes_array, i, op.lineno);
	}
	efree(op_array);
	efree(zv);
	*return_value = *opcodes_array;
}
/* }}} */


/*
 * {{{ public gene_execute::StringRun($codeString)
 */
PHP_METHOD(gene_execute, StringRun)
{
	char *php_script;
	int php_script_len;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &php_script, &php_script_len) == FAILURE) {
		return;
	}
	zend_try {
		zend_eval_stringl(php_script, php_script_len, NULL, "" TSRMLS_CC);
	} zend_catch {
		zend_bailout();
	}zend_end_try();
	RETURN_TRUE;
}
/* }}} */


/*
 * {{{ gene_execute_methods
 */
zend_function_entry gene_execute_methods[] = {
		PHP_ME(gene_execute, GetOpcodes, NULL, ZEND_ACC_PUBLIC)
		PHP_ME(gene_execute, StringRun, NULL, ZEND_ACC_PUBLIC)
		PHP_ME(gene_execute, __construct, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
		{NULL, NULL, NULL}
};
/* }}} */


/*
 * {{{ GENE_MINIT_FUNCTION
 */
GENE_MINIT_FUNCTION(execute)
{
    zend_class_entry gene_execute;
    INIT_CLASS_ENTRY(gene_execute,"gene_execute",gene_execute_methods);
    gene_execute_ce = zend_register_internal_class(&gene_execute TSRMLS_CC);

	//debug
    zend_declare_property_null(gene_execute_ce, GENE_EXECUTE_DEBUG, strlen(GENE_EXECUTE_DEBUG), ZEND_ACC_PUBLIC TSRMLS_CC);
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
