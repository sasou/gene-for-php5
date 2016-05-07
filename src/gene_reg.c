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
  | Author: Sasou  <admin@caophp.com>                                    |
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
#include "gene_reg.h"

zend_class_entry * gene_reg_ce;

/*
 * {{{ gene_reg
 */
PHP_METHOD(gene_reg, __construct)
{

}
/* }}} */

/*
 *  {{{ zval *gene_reg_instance(zval *this_ptr TSRMLS_DC)
 */
zval *gene_reg_instance(zval *this_ptr TSRMLS_DC)
{
	zval *instance = zend_read_static_property(gene_reg_ce, GENE_REG_PROPERTY_INSTANCE, strlen(GENE_REG_PROPERTY_INSTANCE), 1 TSRMLS_CC);

	if (Z_TYPE_P(instance) != IS_OBJECT || !instanceof_function(Z_OBJCE_P(instance), gene_reg_ce TSRMLS_CC)) {
		zval *regs;

		MAKE_STD_ZVAL(instance);
		object_init_ex(instance, gene_reg_ce);

		MAKE_STD_ZVAL(regs);
		array_init(regs);
		zend_update_property(gene_reg_ce, instance, GENE_REG_PROPERTY_REG, strlen(GENE_REG_PROPERTY_REG), regs TSRMLS_CC);
		zend_update_static_property(gene_reg_ce, GENE_REG_PROPERTY_INSTANCE, strlen(GENE_REG_PROPERTY_INSTANCE), instance TSRMLS_CC);
		zval_ptr_dtor(&regs);
		zval_ptr_dtor(&instance);
	}

	return instance;
}
/* }}} */


/*
 *  {{{ public static gene_reg::get($name)
 */
PHP_METHOD(gene_reg, get) {
	char *name;
	int  len;
	zval **ppzval,*reg,*entrys;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &name, &len) == FAILURE) {
		RETURN_NULL();
	}
	reg = gene_reg_instance(NULL TSRMLS_CC);
	entrys	 = zend_read_property(gene_reg_ce, reg, GENE_REG_PROPERTY_REG, strlen(GENE_REG_PROPERTY_REG), 1 TSRMLS_CC);
	if (entrys && Z_TYPE_P(entrys) == IS_ARRAY) {
		if (zend_hash_find(Z_ARRVAL_P(entrys), name, len + 1, (void **) &ppzval) == SUCCESS) {
			RETURN_ZVAL(*ppzval, 1, 0);
		}
	}
	RETURN_NULL();
}
/* }}} */

/*
 *  {{{ public static gene_reg::set($name, $value)
 */
PHP_METHOD(gene_reg, set) {
	zval *value,*reg,*entrys;
	char *name;
	int len;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz", &name, &len, &value) == FAILURE) {
		RETURN_NULL();
	}
	reg = gene_reg_instance(NULL TSRMLS_CC);
	entrys 	 = zend_read_property(gene_reg_ce, reg, GENE_REG_PROPERTY_REG, strlen(GENE_REG_PROPERTY_REG), 1 TSRMLS_CC);
	Z_ADDREF_P(value);
	if (zend_hash_update(Z_ARRVAL_P(entrys), name, len + 1, &value, sizeof(zval *), NULL) == SUCCESS) {
		RETURN_TRUE;
	}
	RETURN_FALSE;
}
/* }}} */

/*
 *  {{{ public static gene_reg::del($name)
 */
PHP_METHOD(gene_reg, del) {
	char *name;
	int len;
	zval *reg,*entrys;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &name, &len) == FAILURE) {
		RETURN_NULL();
	}
	reg = gene_reg_instance(NULL TSRMLS_CC);
	entrys	 = zend_read_property(gene_reg_ce, reg,GENE_REG_PROPERTY_REG, strlen(GENE_REG_PROPERTY_REG), 1 TSRMLS_CC);
	zend_hash_del(Z_ARRVAL_P(entrys), name, len + 1);
	RETURN_TRUE;
}
/* }}} */

/*
 *  {{{ public gene_reg::has($name)
 */
PHP_METHOD(gene_reg, has) {
	char *name;
	int len;
	zval *reg,*entrys;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &name, &len) == FAILURE) {
		RETURN_NULL();
	}
	reg = gene_reg_instance(NULL TSRMLS_CC);
	entrys	 = zend_read_property(gene_reg_ce, reg,GENE_REG_PROPERTY_REG, strlen(GENE_REG_PROPERTY_REG), 1 TSRMLS_CC);
	if (zend_hash_exists(Z_ARRVAL_P(entrys), name, len + 1) == 1) {
		RETURN_TRUE;
	}
	RETURN_FALSE;
}
/* }}} */

/*
 *  {{{ public gene_reg::getInstance(void)
 */
PHP_METHOD(gene_reg, getInstance)
{
	zval *reg = gene_reg_instance(NULL TSRMLS_CC);
	RETURN_ZVAL(reg, 1, 0);
}
/* }}} */

/*
 * {{{ gene_reg_methods
 */
zend_function_entry gene_reg_methods[] = {
		PHP_ME(gene_reg, getInstance, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
		PHP_ME(gene_reg, get, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
		PHP_ME(gene_reg, has, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
		PHP_ME(gene_reg, set, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
		PHP_ME(gene_reg, del, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
		PHP_ME(gene_reg, __construct, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
		{NULL, NULL, NULL}
};
/* }}} */


/*
 * {{{ GENE_MINIT_FUNCTION
 */
GENE_MINIT_FUNCTION(reg)
{
    zend_class_entry gene_reg;
    INIT_CLASS_ENTRY(gene_reg,"gene_reg",gene_reg_methods);
    gene_reg_ce = zend_register_internal_class(&gene_reg TSRMLS_CC);

	//static
    zend_declare_property_null(gene_reg_ce, GENE_REG_PROPERTY_INSTANCE, strlen(GENE_REG_PROPERTY_INSTANCE),  ZEND_ACC_PROTECTED|ZEND_ACC_STATIC TSRMLS_CC);
    zend_declare_property_null(gene_reg_ce, GENE_REG_PROPERTY_REG, strlen(GENE_REG_PROPERTY_REG),  ZEND_ACC_PROTECTED TSRMLS_CC);
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
