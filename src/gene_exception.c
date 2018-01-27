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
#include "gene_exception.h"
#include "gene_router.h"
#include "gene_view.h"
#include "gene_load.h"
#include "gene_common.h"

zend_class_entry * gene_exception_ce;

/** {{{ void gene_exception_error_register(zval *callback TSRMLS_DC)
 */
void gene_exception_error_register(zval *callback TSRMLS_DC) {
	zval *call = NULL,*func = NULL;

	MAKE_STD_ZVAL(func);
	ZVAL_STRING(func, GENE_ERROR_HANDLER, 1);
	if (!callback) {
		MAKE_STD_ZVAL(call);
		if (GENE_G(use_namespace)) {
			ZVAL_STRING(call, GENE_ERROR_FUNC_NAME_NS, 1);
		} else {
			ZVAL_STRING(call, GENE_ERROR_FUNC_NAME, 1);
		}
		gene_zend_func_call_1(func, call);
	} else {
		gene_zend_func_call_1(func, callback);
	}

	zval_ptr_dtor(&func);
	if (call) {
		zval_ptr_dtor(&call);
	}
}

/** {{{ void gene_exception_register(zval *callback TSRMLS_DC)
 */
void gene_exception_register(zval *callback TSRMLS_DC) {
	zval *call = NULL, *func = NULL;

	MAKE_STD_ZVAL(func);
	ZVAL_STRING(func, GENE_EXCEPTION_HANDLER, 1);
	if (!callback) {
		MAKE_STD_ZVAL(call);
		if (GENE_G(use_namespace)) {
			ZVAL_STRING(call, GENE_EXCEPTION_FUNC_NAME_NS, 1);
		} else {
			ZVAL_STRING(call, GENE_EXCEPTION_FUNC_NAME, 1);
		}
		gene_zend_func_call_1(func, call);
	} else {
		gene_zend_func_call_1(func, callback);
	}

	zval_ptr_dtor(&func);
	if (call) {
		zval_ptr_dtor(&call);
	}
}

/** {{{void gene_trigger_error(int type TSRMLS_DC, char *format, ...)
 */
void gene_trigger_error(int type TSRMLS_DC, char *format, ...) {
	va_list args;
	char *message;
	int msg_len;

	va_start(args, format);
	msg_len = vspprintf(&message, 0, format, args);
	va_end(args);

	if (GENE_G(gene_error)) {
#if PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION < 3
		if (!EG(exception)) {
			gene_throw_exception(type, message TSRMLS_CC);
		}
#else
		gene_throw_exception(type, message TSRMLS_CC);
#endif
	} else {
		php_error_docref(NULL TSRMLS_CC, E_RECOVERABLE_ERROR, "%s", message);
	}
	efree(message);
}
/* }}} */

/** {{{ zend_class_entry * gene_get_exception_base(int root TSRMLS_DC)
 */
zend_class_entry * gene_get_exception_base(int root TSRMLS_DC) {
#if can_handle_soft_dependency_on_SPL && defined(HAVE_SPL) && ((PHP_MAJOR_VERSION > 5) || (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 1))
	if (!root) {
		if (!spl_ce_RuntimeException) {
			zend_class_entry **pce;

			if (zend_hash_find(CG(class_table), "runtimeexception", sizeof("RuntimeException"), (void **) &pce) == SUCCESS) {
				spl_ce_RuntimeException = *pce;
				return *pce;
			}
		} else {
			return spl_ce_RuntimeException;
		}
	}
#endif

#if (PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION < 2)
	return zend_exception_get_default();
#else
	return zend_exception_get_default(TSRMLS_C);
#endif
}
/* }}} */

/** {{{ void gene_throw_exception(long code, char *message TSRMLS_DC)
 */
void gene_throw_exception(long code, char *message TSRMLS_DC) {
	zend_class_entry *base_exception = gene_exception_ce;

	zend_throw_exception(base_exception, message, code TSRMLS_CC);
}
/* }}} */

#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION < 3)) || (PHP_MAJOR_VERSION < 5)
/** {{{ proto gene_exception::__construct($mesg = 0, $code = 0, Exception $previous = NULL)
 */
PHP_METHOD(gene_exception, __construct) {
	char *message = NULL;
	zval *object, *previous = NULL;
	int message_len, code = 0;
	int argc = ZEND_NUM_ARGS();

	if (zend_parse_parameters_ex(ZEND_PARSE_PARAMS_QUIET, argc TSRMLS_CC, "|sl!", &message, &message_len, &code, &previous, gene_get_exception_base(0 TSRMLS_CC)) == FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_ERROR, "Wrong parameters for Exception([string $exception [, long $code [, Exception $previous = NULL]]])");
	}

	object = getThis();

	if (message) {
		zend_update_property_string(Z_OBJCE_P(object), object, "message", sizeof("message")-1, message TSRMLS_CC);
	}

	if (code) {
		zend_update_property_long(Z_OBJCE_P(object), object, "code", sizeof("code")-1, code TSRMLS_CC);
	}

	if (previous) {
		zend_update_property(Z_OBJCE_P(object), object, ZEND_STRL("previous"), previous TSRMLS_CC);
	}
}
/* }}} */

/** {{{ proto gene_exception::getPrevious(void)
 */
PHP_METHOD(gene_exception, getPrevious) {
	zval *prev = zend_read_property(Z_OBJCE_P(getThis()), getThis(), ZEND_STRL("previous"), 1 TSRMLS_CC);
	RETURN_ZVAL(prev, 1, 0);
}
/* }}} */
#endif

/** {{{ public gene_exception::setErrorHandler(string $callbacak[, int $error_types = E_ALL | E_STRICT ] )
 */
PHP_METHOD(gene_exception, setErrorHandler) {
	zval *callback = NULL;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &callback) == FAILURE) {
		return;
	}
	gene_exception_error_register(callback TSRMLS_CC);
	RETURN_TRUE;
}
/* }}} */

/** {{{ public gene_exception::setExceptionHandler(string $callbacak[, int $error_types = E_ALL | E_STRICT ] )
 */
PHP_METHOD(gene_exception, setExceptionHandler) {
	zval *callback = NULL;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|z", &callback) == FAILURE) {
		return;
	}
	gene_exception_register(callback TSRMLS_CC);
	RETURN_TRUE;
}
/* }}} */

/*
 * {{{ gene_exception::doError
 */
PHP_METHOD(gene_exception, doError) {
	char *msg, *file;
	zval *params = NULL;
	long code = 0, line = 0, msg_len, file_len;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ls|slz", &code, &msg, &msg_len, &file, &file_len, &line, &params) == FAILURE) {
		RETURN_NULL();
	}
	if (GENE_G(gene_error) == 1) {
		gene_trigger_error(code, "%s", msg);
	}
	RETURN_TRUE;
}
/* }}} */

/*
 * {{{ gene_exception_methods
 */
zend_function_entry gene_exception_methods[] = {
	PHP_ME(gene_exception, setErrorHandler, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(gene_exception, setExceptionHandler, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(gene_exception, doError, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION < 3)) || (PHP_MAJOR_VERSION < 5)
	PHP_ME(gene_exception, __construct, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(gene_exception, getPrevious, NULL, ZEND_ACC_PUBLIC)
#endif
	{ NULL, NULL, NULL }
};
/* }}} */

/*
 * {{{ GENE_MINIT_FUNCTION
 */
GENE_MINIT_FUNCTION(exception) {
	zend_class_entry gene_exception;
	GENE_INIT_CLASS_ENTRY(gene_exception, "Gene_Exception", "Gene\\Exception",
			gene_exception_methods);
	gene_exception_ce = zend_register_internal_class_ex(&gene_exception,
			gene_get_exception_base(0 TSRMLS_CC), NULL TSRMLS_CC);
	zend_declare_property_null(gene_exception_ce, ZEND_STRL("message"),
			ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_long(gene_exception_ce, ZEND_STRL("code"), 0,
			ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(gene_exception_ce, ZEND_STRL("previous"),
			ZEND_ACC_PROTECTED TSRMLS_CC);

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
