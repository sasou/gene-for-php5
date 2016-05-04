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

#ifndef GENE_ROUTER_H
#define GENE_ROUTER_H
#define GENE_ROUTER_SAFE	"safe"
#define GENE_ROUTER_PREFIX	"prefix"
#define GENE_ROUTER_ROUTER_EVENT ":re"
#define GENE_ROUTER_ROUTER_TREE ":rt"
#define GENE_ROUTER_LEAF_RUN "%s/leaf/run"
#define GENE_ROUTER_LEAF_HOOK "%s/leaf/hook"
#define GENE_ROUTER_LEAF_RUN_L "%s/%s/leaf/run"
#define GENE_ROUTER_LEAF_HOOK_L "%s/%s/leaf/hook"
#define GENE_ROUTER_CHIRD "chird/"
#define GENE_ROUTER_CONTENT_B  "if(count($gene_url) == 0) $gene_url = array(NULL);$gene_h=call_user_func_array(array('%s','%s'),$gene_url);if(isset($gene_h) && ($gene_h == 0))return;"
#define GENE_ROUTER_CONTENT_A  "if(isset($gene_m)) $gene_p = array(0=>$gene_m);else $gene_p = array(NULL);call_user_func_array(array('%s','%s'),$gene_p);"
#define GENE_ROUTER_CONTENT_M  "if(count($gene_url) == 0) $gene_url = array(NULL);$gene_m=call_user_func_array(array('%s','%s'),$gene_url);"
#define GENE_ROUTER_CONTENT_H  "if(count($gene_url) == 0) $gene_url = array(NULL);call_user_func_array(array('%s','%s'),$gene_url);"
#define GENE_ROUTER_CONTENT_FB  "if(count($gene_url) == 0) $gene_url = array(NULL);$gene_h=call_user_func_array(%s,$gene_url);if(isset($gene_h) && ($gene_h == 0))return;"
#define GENE_ROUTER_CONTENT_FA  "if(isset($gene_m)) $gene_p = array(0=>$gene_m);else $gene_p = array(NULL);call_user_func_array(%s,$gene_p);"
#define GENE_ROUTER_CONTENT_FM  "if(count($gene_url) == 0) $gene_url = array(NULL);$gene_m=call_user_func_array(%s,$gene_url);"
#define GENE_ROUTER_CONTENT_FH  "if(count($gene_url) == 0) $gene_url = array(NULL);call_user_func_array(%s,$gene_url);"




extern zend_class_entry *gene_router_ce;

void get_router_content_run(char *methodin,char *pathin,zval *safe TSRMLS_DC);

GENE_MINIT_FUNCTION(router);

#endif
