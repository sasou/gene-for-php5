<?php
namespace Controllers;
class Index extends \gene\Controller{
	function run(){
		echo ' index run ';
		$test = ' sasou ';
		$this->display("run");
	}
	
	public static function error(gene_exception $e){
		\gene\router::display("error"); 
	}
}