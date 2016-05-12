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

#ifndef PHP_GENE_H
#define PHP_GENE_H

extern zend_module_entry gene_module_entry;
#define phpext_gene_ptr &gene_module_entry

#define PHP_GENE_VERSION "1.0.0"

#ifdef PHP_WIN32
#	define PHP_GENE_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_GENE_API __attribute__ ((visibility("default")))
#else
#	define PHP_GENE_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

#define GENE_MINIT_FUNCTION(module)   	    ZEND_MINIT_FUNCTION(gene_##module)
#define GENE_MSHUTDOWN_FUNCTION(module)   	ZEND_MSHUTDOWN_FUNCTION(gene_##module)
#define GENE_RINIT_FUNCTION(module)   	    ZEND_RINIT_FUNCTION(gene_##module)
#define GENE_RSHUTDOWN_FUNCTION(module)   	ZEND_RSHUTDOWN_FUNCTION(gene_##module)
#define GENE_MINFO_FUNCTION(module)   	PHP_MINFO_FUNCTION(module)
#define GENE_STARTUP(module)	 		ZEND_MODULE_STARTUP_N(gene_##module)(INIT_FUNC_ARGS_PASSTHRU)


PHP_MINIT_FUNCTION(gene);
PHP_MSHUTDOWN_FUNCTION(gene);
PHP_RINIT_FUNCTION(gene);
PHP_RSHUTDOWN_FUNCTION(gene);
PHP_MINFO_FUNCTION(gene);



ZEND_BEGIN_MODULE_GLOBALS(gene)
	char		*base_uri;
	char 		*directory;
	char		*method;
	char 		*path;
	char 		*router_path;
	char 		*app_key;
	HashTable	*cache;
	HashTable	*cache_easy;
ZEND_END_MODULE_GLOBALS(gene)

extern ZEND_DECLARE_MODULE_GLOBALS(gene);

/* In every utility function you add that needs to use variables 
   in php_gene_globals, call TSRMLS_FETCH(); after declaring other 
   variables used by that function, or better yet, pass in TSRMLS_CC
   after the last function argument and declare your utility function
   with TSRMLS_DC after the last declared argument.  Always refer to
   the globals in your function as GENE_G(variable).  You are 
   encouraged to rename these macros something shorter, see
   examples in any other php module directory.
*/

#ifdef ZTS
#define GENE_G(v) TSRMG(gene_globals_id, zend_gene_globals *, v)
#else
#define GENE_G(v) (gene_globals.v)
#endif

#endif	/* PHP_GENE_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
