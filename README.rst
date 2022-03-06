wikidice
--------

This software serves a random Wikipedia article within a specific category. Wikipedia itself has a Random Page feature, but it chooses a random article out of the entire site. I'm not interested in everything. If I want to learn a random topic just under a specific category, there's no way to do that. This is what `wikidice` is for. You can choose whichever category (as long as it has a "Category:" page) and a recursion depth for how many levels of subcategories you want to choose from. Try it out!

Setting up the database
=======================

All requests come from a MySQL/MariaDB database that locally mirrors a couple of `Wikipedia's database dumps <https://dumps.wikimedia.org/>`_.

We need to import two tables from Wikipedia's dumps:

1. `Categorylinks table <https://www.mediawiki.org/wiki/Manual:Categorylinks_table>`_ contains mappings from pages to categories to which they belong.
2. `Page table <https://www.mediawiki.org/wiki/Manual:Page_table>`_, for our purposes, contains the page title associated with a numeric page ID.

For example: the latest dump of the `categorylinks` table as of writing is: `https://dumps.wikimedia.org/enwiki/20220301/enwiki-20220301-categorylinks.sql.gz`. The latest dump of the `page` table as of writing is: `https://dumps.wikimedia.org/enwiki/20220301/enwiki-20220301-page.sql.gz`. Download, extract with gunzip(1), and import into a new MySQL database.

To make lookups faster, create a third table as follows::

  > describe page_cat_ids;
  +---------+------------------------------+------+-----+---------+-------+
  | Field   | Type                         | Null | Key | Default | Extra |
  +---------+------------------------------+------+-----+---------+-------+
  | page_id | int(8) unsigned              | NO   | MUL | NULL    |       |
  | cat_id  | int(8) unsigned              | NO   | MUL | NULL    |       |
  | cl_type | enum('page','subcat','file') | NO   |     | page    |       |
  +---------+------------------------------+------+-----+---------+-------+

  -- How to create this database:
  
  CREATE TABLE page_cat_ids (
  page_id int(8) unsigned not null,
  cat_id int(8) unsigned not null,
  cl_type enum('page', 'subcat', 'file') default 'page'
  );

  -- Then populate it (after you import `page` and `categorylinks` tables).
  -- This may take a long time and use a lot of memory.
  -- If it fails, try setting `innodb_buffer_pool_size = 8G` in your MySQL/MariaDB config.

  INSERT INTO page_cat_ids (page_id, cat_id, cl_type)
  SELECT cl.cl_from AS page_id, page.page_id AS cat_id, cl.cl_type AS cl_type
  FROM categorylinks cl
  INNER JOIN page ON page.page_title = cl.cl_to AND page.page_namespace = 14;

Disclaimer
==========

Not associated or affiliated with Wikipedia in any way.


License
=======

Apache-2.0
