#php-gene

	Simple, high performance,C extension framework for php！

版本 1.0.0

简单、高性能的php c扩展框架！
框架的核心是gene_application类，加载配置文件并启动：

	<?php
	$app = new gene_application('sdfsdf');
	$app->load("router.ini.php")
	    ->load("config.ini.php")
	    ->run();

框架的基础是一个高性能的进程缓存模块，基于缓存模块，实现了一个高性能的强大路由解析以及配置缓存；
路由强大灵活，支持回调、rest、http请求方式（get,post,put,patch,delete,trace,connect,options,head）等：

	<?php
	$router = new gene_router();
	$router->clear()
		->get("/",function(){
				echo "index";
			})
		->group("/admin")
			->get("/:name/",function($abc){
				echo $abc;
			})
			->get("/:name.:ext",function($abc){
				echo $abc;
			})
			->get("/:name/sasoud",function(){
				echo 'dd';
			},"name")
			->get("/blog/:ext/baidu",function(){
				echo 'baidu';
				return array('sdfasd'=>'baidu.edu.com');
			},"auth@clearAll")
			->get("/",function(){
				echo 'admin';
			},'adminauth')
		->group()
		->get("/index",function(){
			echo 'index';
		})
		->error(401,"gene_cache@get")
		->hook("auth",function(){
			echo " auth ";
		})
		->hook("before", function(){
			echo " before ";
		})
		->hook("after", function($params){
			echo " after ";
			if(is_array($params))var_dump($params);
		});

配置类支持bool、int、long、string、array、常量等数据类型：

	<?php
	$config = new gene_config();
	$config->clear();
	$config>set("dsfsdfsd",array('_url'=>array('sd'=>'sdfsdf222','sds'=>'sdfsf678'),'port'=>3307));
	支持快捷调用（.分隔）：
	$config->get("dsfsdfsd.port");
	
安装：
	
	phpize
	./configure --with-config-file-path=PATH --disable-all --enable-cli --enable-cgi --enable-gene=shared
	make
	make install
	
DEMO：
	
	index.php 启动文件
	config.ini.php 配置文件
	router.inc.php 路由文件
	
测试：

	测试网站：php-gene.com
	http://php-gene.com/
	可测试路由实例：
	http://php-gene.com/demo/admin
	http://php-gene.com/demo/admin.html
	http://php-gene.com/demo/admin/demo.jpg
	http://php-gene.com/demo/admin/ajax.js
	http://php-gene.com/demo/admin/blog/test/baidu