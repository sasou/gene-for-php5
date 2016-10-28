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

#ifndef GENE_EXCEPTION_H
#define GENE_EXCEPTION_H
#define GENE_ERROR_FUNC_NAME 		"gene_exception::doError"
#define GENE_EXCEPTION_FUNC_NAME 	"gene_exception::doException"
#define GENE_ERROR_FUNC_NAME_NS 	"gene\\exception::doError"
#define GENE_EXCEPTION_FUNC_NAME_NS "gene\\exception::doException"
#define GENE_EXCEPTION_EX "_ex"
#define HTML_ERROR_CONTENT "echo ' [Uncaught ',get_class($e),']:',$e->getMessage(),'File:***',substr($e->getFile(),-(ceil(strlen($e->getFile())*0.6))),' ',$e->getLine();"

extern zend_class_entry *gene_exception_ce;
int gene_exception_error_register(zval *callback,zval *error_type TSRMLS_DC);
int gene_exception_register(zval *callback,zval *error_type TSRMLS_DC);
void gene_throw_exception(long code, char *message TSRMLS_DC);
int gene_exception_error_register(zval *callback,zval *error_type TSRMLS_DC);
void gene_trigger_error(int type TSRMLS_DC, char *format, ...);

GENE_MINIT_FUNCTION(exception);

#endif
