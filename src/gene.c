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
#include "ext/standard/info.h"
#include "Zend/zend_API.h"
#include "zend_exceptions.h"


#include "php_gene.h"
#include "gene_application.h"
#include "gene_load.h"
#include "gene_config.h"
#include "gene_router.h"
#include "gene_execute.h"
#include "gene_cache.h"
#include "gene_reg.h"
#include "gene_controller.h"
#include "gene_request.h"
#include "gene_response.h"
#include "gene_session.h"
#include "gene_view.h"
#include "gene_exception.h"


ZEND_DECLARE_MODULE_GLOBALS(gene);

/** {{{ PHP_INI
 */
PHP_INI_BEGIN()
	STD_PHP_INI_BOOLEAN("gene.namespace",   	"0", PHP_INI_SYSTEM, OnUpdateBool, use_namespace, zend_gene_globals, gene_globals)
PHP_INI_END();
/* }}} */


/* {{{ php_gene_init_globals
 */
static void php_gene_init_globals()
{
	TSRMLS_FETCH();
	GENE_G(directory) = NULL;
	GENE_G(app_root) = NULL;
	GENE_G(app_view) = NULL;
	GENE_G(app_ext) = NULL;
	GENE_G(method) = NULL;
	GENE_G(path) = NULL;
	GENE_G(router_path) = NULL;
	GENE_G(app_key) = NULL;
	GENE_G(cache) = NULL;
	GENE_G(cache_easy) = NULL;
	GENE_G(auto_load_fun) = NULL;
	gene_cache_init(TSRMLS_C);
}
/* }}} */

/* {{{ php_gene_init_auto_globals
 */
static void php_gene_init_auto_globals()
{
	zend_bool jit_initialization;
	TSRMLS_FETCH();
#if (PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION < 4)
	jit_initialization = (PG(auto_globals_jit) && !PG(register_globals) && !PG(register_long_arrays));
#else
	jit_initialization = PG(auto_globals_jit);
#endif

	if (jit_initialization) {
		zend_is_auto_global(ZEND_STRL("_ENV") TSRMLS_CC);
		zend_is_auto_global(ZEND_STRL("_SERVER") TSRMLS_CC);
		zend_is_auto_global(ZEND_STRL("_REQUEST") TSRMLS_CC);
	}
}
/* }}} */

/** {{{ PHP_GINIT_FUNCTION
*/
PHP_GINIT_FUNCTION(gene)
{
	gene_globals->gene_error = 1;
	gene_globals->gene_exception = 0;
	gene_globals->show_exception = 0;
}
/* }}} */

/*
 * {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(gene)
{
	php_gene_init_globals();
	REGISTER_INI_ENTRIES();

	/* startup components */
	GENE_STARTUP(application);
	GENE_STARTUP(load);
	GENE_STARTUP(reg);
	GENE_STARTUP(config);
	GENE_STARTUP(router);
    GENE_STARTUP(execute);
    GENE_STARTUP(cache);
	GENE_STARTUP(controller);
	GENE_STARTUP(request);
	GENE_STARTUP(response);
	GENE_STARTUP(session);
	GENE_STARTUP(view);
	GENE_STARTUP(exception);

    return SUCCESS;
}
/* }}} */

/*
 * {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(gene)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/
    if (GENE_G(directory)) {
    	efree(GENE_G(directory));
    	GENE_G(directory) = NULL;
    }
    if (GENE_G(app_root)) {
    	efree(GENE_G(app_root));
    	GENE_G(app_root) = NULL;
    }
    if (GENE_G(app_view)) {
    	efree(GENE_G(app_view));
    	GENE_G(app_view) = NULL;
    }
    if (GENE_G(app_ext)) {
    	efree(GENE_G(app_ext));
    	GENE_G(app_ext) = NULL;
    }
    if (GENE_G(method)) {
    	efree(GENE_G(method));
    	GENE_G(method) = NULL;
    }
    if (GENE_G(path)) {
    	efree(GENE_G(path));
    	GENE_G(path) = NULL;
    }
    if (GENE_G(router_path)) {
    	efree(GENE_G(router_path));
    	GENE_G(router_path) = NULL;
    }
    if (GENE_G(auto_load_fun)) {
    	efree(GENE_G(auto_load_fun));
    	GENE_G(auto_load_fun) = NULL;
    }
	if (GENE_G(cache)) {
		zend_hash_destroy(GENE_G(cache));
		pefree(GENE_G(cache), 1);
		GENE_G(cache) = NULL;
	}
	if (GENE_G(cache_easy)) {
		zend_hash_destroy(GENE_G(cache_easy));
		pefree(GENE_G(cache_easy), 1);
		GENE_G(cache_easy) = NULL;
	}
	return SUCCESS;
}

/* }}} */

/*
 * {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(gene)
{
	php_gene_init_auto_globals();
	return SUCCESS;
}
/* }}} */

/*
 * {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(gene)
{
    if (GENE_G(directory)) {
    	efree(GENE_G(directory));
    	GENE_G(directory) = NULL;
    }
    if (GENE_G(app_root)) {
    	efree(GENE_G(app_root));
    	GENE_G(app_root) = NULL;
    }
    if (GENE_G(app_view)) {
    	efree(GENE_G(app_view));
    	GENE_G(app_view) = NULL;
    }
    if (GENE_G(app_ext)) {
    	efree(GENE_G(app_ext));
    	GENE_G(app_ext) = NULL;
    }
    if (GENE_G(method)) {
    	efree(GENE_G(method));
    	GENE_G(method) = NULL;
    }
    if (GENE_G(path)) {
    	efree(GENE_G(path));
    	GENE_G(path) = NULL;
    }
    if (GENE_G(router_path)) {
    	efree(GENE_G(router_path));
    	GENE_G(router_path) = NULL;
    }
    if (GENE_G(auto_load_fun)) {
    	efree(GENE_G(auto_load_fun));
    	GENE_G(auto_load_fun) = NULL;
    }
    if (GENE_G(app_key)) {
    	efree(GENE_G(app_key));
    	GENE_G(app_key) = NULL;
    }
    gene_cache_del(PHP_GENE_URL_PARAMS, strlen(PHP_GENE_URL_PARAMS) TSRMLS_CC);
	return SUCCESS;
}
/* }}} */

/*
 * {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(gene)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "gene support", "enabled");
	php_info_print_table_row(2, "Author:", " sasou <admin@php-gene.com>");
	php_info_print_table_row(2, "Web site:", " http://www.php-gene.com");
	php_info_print_table_row(2, "Version:", PHP_GENE_VERSION);
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
}
/* }}} */


PHP_FUNCTION(gene_urlParams){
	zval *cache = NULL;
	int keyString_len;
	char *keyString = NULL;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|s", &keyString, &keyString_len) == FAILURE) {
		return;
	}
    cache = gene_cache_get_by_config(PHP_GENE_URL_PARAMS, strlen(PHP_GENE_URL_PARAMS), keyString TSRMLS_CC);
    if (cache) {
    	RETURN_ZVAL(cache, 1, 1);
    }
	RETURN_NULL();
}

/* {{{ gene_functions[]
 *
 * Every user visible function must have an entry in gene_functions[].
 */
zend_function_entry gene_functions[] = {
	PHP_FE(gene_urlParams,NULL)
	PHP_FE_END
};
/* }}} */


/*
 * {{{ gene_module_entry
 */
#ifdef COMPILE_DL_GENE
ZEND_GET_MODULE(gene)
#endif
/* }}} */

/** {{{ module depends
 */
#if ZEND_MODULE_API_NO >= 20050922
zend_module_dep gene_deps[] = {
	ZEND_MOD_REQUIRED("spl")
	ZEND_MOD_REQUIRED("pcre")
	ZEND_MOD_OPTIONAL("session")
	{NULL, NULL, NULL}
};
#endif
/* }}} */

/** {{{ gene_module_entry
*/
zend_module_entry gene_module_entry = {
#if ZEND_MODULE_API_NO >= 20050922
	STANDARD_MODULE_HEADER_EX, NULL,
	gene_deps,
#else
	STANDARD_MODULE_HEADER,
#endif
	"gene",
	gene_functions,
	PHP_MINIT(gene),
	PHP_MSHUTDOWN(gene),
	PHP_RINIT(gene),
	PHP_RSHUTDOWN(gene),
	PHP_MINFO(gene),
	PHP_GENE_VERSION,
	PHP_MODULE_GLOBALS(gene),
	PHP_GINIT(gene),
	NULL,
	NULL,
	STANDARD_MODULE_PROPERTIES_EX
};
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
