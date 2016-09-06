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

#ifndef GENE_CACHE_H
#define GENE_CACHE_H
#define GENE_CACHE_SAFE	"safe"
#define PHP_GENE_URL_PARAMS ":gene_url"

struct _gene_cache_container {
	long ctime;
	int validity;
	int status;
	zval *data;
};

typedef struct _gene_cache_container gene_cache_container;

struct _gene_cache_container_easy {
	long stime;
	long ftime;
	int validity;
	int status;
};

typedef struct _gene_cache_container_easy gene_cache_container_easy;

extern zend_class_entry *gene_cache_ce;



void gene_cache_init(TSRMLS_D);
void gene_cache_set(char *keyString, int keyString_len, zval *zvalue, int validity TSRMLS_DC);
zval * gene_cache_get(char *keyString, int keyString_len TSRMLS_DC);
zval * gene_cache_get_quick(char *keyString, int keyString_len TSRMLS_DC);
zval * gene_cache_get_by_config(char *keyString, int keyString_len,char *path TSRMLS_DC);
void gene_cache_set_by_router(char *keyString, int keyString_len, char *path, zval *zvalue, int validity TSRMLS_DC);
zval * gene_cache_set_val(zval *val, char *keyString, int keyString_len, zval *zvalue TSRMLS_DC);
long gene_cache_getTime(char *keyString, int keyString_len TSRMLS_DC);
int gene_cache_exists(char *keyString, int keyString_len TSRMLS_DC);
int gene_cache_del(char *keyString, int keyString_len TSRMLS_DC);
void gene_cache_set_easy(char *keyString, int keyString_len, int time, int validity TSRMLS_DC);
gene_cache_container_easy * gene_cache_get_easy(char *keyString, int keyString_len TSRMLS_DC);
zval * gene_cache_zval_losable(zval *zvalue TSRMLS_DC);


#define GENE_CZAL(z, v)					\
	do {								\
		(z)->value = (v)->value;		\
		Z_TYPE_P(z) = Z_TYPE_P(v);		\
	} while (0)


GENE_MINIT_FUNCTION(cache);

#endif
