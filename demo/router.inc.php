<?php
return new Gene_RouterDispatch(
array(
	'GET' => array(
		':' => array(), 
		'LEAF' => array(
			0 => function(){echo "Hello world !!!";}, 
			1 => array()
		), 
		'index' => array(
			':' => array(), 
			'html' => array(
				':' => array(), 
				'LEAF' => array(
					0 => function(){echo "Good Lucky!";}, 
					1 => array())
			), 
			'php' => array(
				':' => array(), 
				'LEAF' => array(
					0 => function(){echo "Good Lucky!";}, 
					1 => array())
				)
		), 
		'hello' => array(
			':' => array(
				'name' => array(
					':' => array(
					'ext' => array(
						':' => array(), 
						'LEAF' => array(
							0 => function($name, $ext){if ('js' == $ext || 'json' == $ext) return array('name'=>$name);return array('code'=>1, 'msg'=>'error message...');}, 
							1 => array(0 => 'auth')
							)
						)
					), 
					'LEAF' => array(0 => array(0 => new Handler(array()), 1 => 'hello'), 1 => array()), 
					'again' => array(
							':' => array(), 
							'LEAF' => array(0 => array(0 => 'Handler', 1 => 'hello_again'), 1 => array(0 => 'auth')
							)
						)
				)
			)
		)
	), 
	'POST' => array(
		':' => array(), 
		'index' => array(
			':' => array(), 
			'html' => array(
					':' => array(), 
					'LEAF' => array(0 => function(){echo "Good Lucky!";}, 1 => array()
				)
			), 
			'php' => array(
				':' => array(), 
				'LEAF' => array(0 => function(){echo "Good Lucky!";}, 1 => array())
				)
			), 
		'hello' => array(
		':' => array(), 
		'LEAF' => array(
			0 => array(0 => new Handler(array()), 1 => 'hello'), 
			1 => array(0 => 'auth')
			)
		)
	 )
), array('error:401' => function($message){
    header('Location: /login', true, 401);
    die($message);
}, 'error:405' => function($message){
    header('Location: /hello/world', true, 405);
    die($message);
}, 'error:406' => function($message){
    die($message);
}, 'hook:auth' => function($params){
    if ('lloyd' == $params['name'])
        return $params;
    $params['router']->error(401, 'Forbiden');
}, 'hook:after' => function($result, $router){
    if ($result) {
        header('Content-type: application/'. (isset($_GET['jsoncallback'])?'javascript':'json'));
        if (isset($_GET['jsoncallback']))
            print $_GET['jsoncallback']. '('. json_encode($result). ')';
        else print json_encode($result);
    }
}, 'hook:before' => function($params){
    //$params['name'] = 'lloydzhou';
    return $params;
}));