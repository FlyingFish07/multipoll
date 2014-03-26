<?php

/**
 * @file
 * User page callbacks for the poll module.
 */

/**
 * Menu callback to provide a simple list of all polls available.
 */
function multipoll_page() {
  $polls_per_page = 15;

  $count_select = db_select('node', 'n');
  $count_select->addExpression('COUNT(*)', 'expression');
  $count_select->join('multipoll', 'p', 'p.nid = n.nid');
  $count_select->condition('n.status', 1);

  // List all polls.
  $select = db_select('node', 'n');
  $select->join('multipoll', 'p', 'p.nid = n.nid');
  $select->join('multipoll_choice', 'c', 'c.nid = n.nid');
  $select->addExpression('SUM(c.chvotes)', 'votes');
  $select = $select->fields('n', array('nid', 'title', 'created'))
    ->fields('p', array('active'))
    ->condition('n.status', 1)
    ->orderBy('n.created', 'DESC')
    ->groupBy('n.nid')
    ->groupBy('n.title')
    ->groupBy('p.active')
    ->groupBy('n.created')
    ->extend('PagerDefault')
    ->limit($polls_per_page)
    ->addTag('node_access');
  $select->setCountQuery($count_select);
  $queried_nodes = $select->execute()
    ->fetchAllAssoc('nid');

  $output = '<ul>';
  foreach ($queried_nodes as $node) {
    $output .= '<li>' . l($node->title, "node/$node->nid") . ' - ' . format_plural($node->votes, '1 vote', '@count votes') . ' - ' . ($node->active ? t('open') : t('closed')) . '</li>';
  }
  $output .= '</ul>';
  $output .= theme('pager');
  return $output;
}

/**
 * Callback for the 'votes' tab for polls you can see other votes on
 */
function multipoll_votes($node) {
  //使用jquery-ui
  drupal_add_library('system','ui.dialog');
  drupal_add_library('system','ui.button');
  drupal_add_js(drupal_get_path('module', 'multipoll') .'/multipoll.js');

  $votes_per_page = 20;
  drupal_set_title($node->title);
  $can_cancel = user_access('cancel the multipoll vote') ? TRUE : FALSE;

  $header[] = array('data' => t('Visitor'), 'field' => 'u.name');
  $header[] = t('Vote Number');
  $header[] = array('data' => t('Timestamp'), 'field' => 'pv.timestamp', 'sort' => 'desc');
  if($can_cancel){
    $header[] = T('Cancel');
  }

  $select = db_select('multipoll_vote', 'pv')->extend('PagerDefault')->extend('TableSort');
  $select->join('users', 'u', 'pv.uid = u.uid');
  $queried_votes = $select
    ->addTag('translatable')
    ->fields('pv', array('vtid', 'chids', 'uid', 'hostname', 'timestamp', 'nid'))
    ->fields('u', array('name'))
    ->condition('pv.nid', $node->nid)
    ->limit($votes_per_page)
    ->orderByHeader($header) //按头来排序
    ->execute();
  $rows = array();
  foreach ($queried_votes as $vote) {
    $cancel_out = "";
    if($can_cancel){
      //这里必须拆成2行来些，否则会报错
      $elements = drupal_get_form('multipoll_cancel_form', $vote->vtid);
      $cancel_out = drupal_render($elements);
    }
    $rows[] = array(
      $vote->name ? theme('username', array('account' => $vote)) : check_plain($vote->hostname),
      l(t("@vote_num Votes",array("@vote_num"=>substr_count($vote->chids, ',') + 1)), "multipoll/vote/$vote->vtid",array('attributes' => array('class' => array('mpvote-reslut')))),
      format_date($vote->timestamp),
      $cancel_out
    );

  }
  $build['poll_votes_table'] = array(
    '#theme' => 'table',
    '#header' => $header,
    '#rows' => $rows,
    '#prefix' => t('This table lists all the recorded votes for this poll. If anonymous users are allowed to vote, they will be identified by the IP address of the computer they used when they voted.'),
  );
  $build['poll_votes_pager'] = array('#theme' => 'pager');
  //定义点击查看投票结果时的dialog内容存放位置
  $build['poll_dialog'] = array(
    '#prefix' => '<div id="poll-dialog">',
    '#suffix' => '</div>',
  );
  return $build;
}

/**
 * Callback for the 'results' tab for polls you can vote on
 */
function multipoll_results($node) {
  drupal_set_title($node->title);
  $node->show_results = TRUE;
  return node_show($node);
}
/**
 * 加载子函数
 * @param  $vtid
 * @return $vote
 */
function _mpvote_load($vtid){
  $vote = db_query("SELECT chids, nid, uid, hostname, timestamp FROM {multipoll_vote} WHERE vtid = :vtid", array(':vtid' => $vtid))->fetchObject();
  $chids = explode(',', $vote->chids);
  $select = db_select('multipoll_choice', 'mc');
  $select->join('node', 'n', 'n.nid = mc.nid');
  $select->fields('n', array('nid', 'title', 'created'))
          ->fields('mc', array('chid','chtext'))
          ->condition('mc.chid', $chids);
  $queried_choices = $select->execute();
  $vote->choices = array();
  foreach ($queried_choices as $choice) {
    $vote->choices [] = $choice;
  }
  $vote->node = new stdClass();
  $vote->node->nid = $vote->choices[0]->nid;
  $vote->node->title = $vote->choices[0]->title;
  $vote->node->created = $vote->choices[0]->created;
  return $vote;
}
/**
 * 生成返回结果
 * @param $vote
 */
function multipoll_vote_cntnt($vtid){
  $vote=_mpvote_load($vtid);
  $node_title = $vote->node->title;
  $node_href = url("node/".$vote->node->nid);
  $choices = array();
  foreach($vote->choices as $choice){
    $choices[]=$choice->chtext;
  }
  // Check to see if jQuery made the call.  The AJAX call used
  // the POST method and passed in the key/value pair js = 1.
  if (! empty ( $_POST ['js'] )) {
    // jQuery made the call
    // This will return results to jQuery's request
    drupal_json_output ( array (
        'node_title' => $node_title,
        'node_href' => $node_href,
        'voted' => t ( 'Vote Info' ),
        'choices' => $choices,
    ) );
    exit ();
  }
  // It was a non-JavaScript call. return the multipoll-one-vote.tpl.php
  return theme('multipoll_one_vote',array('vote'=>$vote));
}

/**
 * Builds the cancel form for a poll.
 *
 * @ingroup forms
 * @see poll_cancel()
 */
function multipoll_cancel_form($form, &$form_state, $vtid) {
  $form_state['cache'] = TRUE;

  // Store the nid so we can get to it in submit functions.
  $form['#vtid'] = $vtid;

  $form['actions'] = array(
      '#type' => 'actions',
      '#attributes'=>array('class' => array('multipoll-cancel'))
  );
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Cancel this vote'),
    '#submit' => array('multipoll_cancel')
  );

  return $form;
}

/**
 * Submit callback for multipoll_cancel_form().
 */
function multipoll_cancel($form, &$form_state) {
  $vote = db_query("SELECT vtid, chids, nid, uid, hostname, timestamp FROM {multipoll_vote} WHERE vtid = :vtid", array(':vtid' => $form['#vtid']))->fetchObject();

  db_delete('multipoll_vote')
    ->condition('vtid', $vote->vtid)
    ->execute();

  // Subtract one to the votes.
  db_update('multipoll_choice')
    ->expression('chvotes', 'chvotes - 1')
    ->condition('chid', explode(',', $vote->chids))
    ->execute();

  drupal_set_message(t('This vote was cancelled.'));
}

