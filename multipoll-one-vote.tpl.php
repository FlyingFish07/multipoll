<?php

/**
 * @file
 * Default theme implementation to display voting form for a poll.
 *
 * - $vote: 这个投票的信息.
 *
 * @ingroup themeable
 */
?>
<h1><?php print t ( 'Vote Info' )?></h1>
<div class="one-vote">
  <h2><?php echo l($vote->node->title,"node/".$vote->node->nid) ?></h2>
  <?php foreach($vote->choices as $choice){
     print '<p>'.$choice->chtext.'</p>';
  }?>
</div>