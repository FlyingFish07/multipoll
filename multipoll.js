// 文件加载完执行
jQuery(document).ready(function() {
	//添加button
	jQuery( "input[type=submit], button" )
    	.button();
	// 注册点击事件
	jQuery('a.mpvote-reslut').click(function() {
		// ajax回调函数
		var voteShow = function(data) {
			var rslt = '<h3><a href="'+data.node_href+'">'+data.node_title+'</a></h3>';
			for(i in data.choices){
				rslt += '<p>'+data.choices[i]+'</p>'
			}
			jQuery("#poll-dialog").attr("title",data.voted);
			jQuery("#poll-dialog").html(rslt);
			//打开dialog,并使用modal禁止点击页面的其他要素
			jQuery("#poll-dialog").dialog({modal: true});
		}
		//调用ajax，如果成功调用回调函数
		jQuery.ajax({
			type : 'POST', // 使用POST.
			url : this.href,
			dataType : 'json',
			success : voteShow,//回调函数
			data : 'js=1' //区分是ajax还是普通页面调用.
		});
		// 防止浏览器继续处理点击事件.
		return false;
	});
});
