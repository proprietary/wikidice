#include <gtest/gtest.h>
#include <sstream>
#include <spdlog/spdlog.h>

#include "bounded_string_ring.h"
#include "sql_parser.h"

using namespace net_zelcon::wikidice;

std::string category_table_head = R"(
/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8mb4 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

--
-- Table structure for table `category`
--

DROP TABLE IF EXISTS `category`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `category` (
  `cat_id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `cat_title` varbinary(255) NOT NULL DEFAULT '',
  `cat_pages` int(11) NOT NULL DEFAULT 0,
  `cat_subcats` int(11) NOT NULL DEFAULT 0,
  `cat_files` int(11) NOT NULL DEFAULT 0,
  PRIMARY KEY (`cat_id`),
  UNIQUE KEY `cat_title` (`cat_title`),
  KEY `cat_pages` (`cat_pages`)
) ENGINE=InnoDB AUTO_INCREMENT=249284673 DEFAULT CHARSET=binary ROW_FORMAT=COMPRESSED;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `category`
--

/*!40000 ALTER TABLE `category` DISABLE KEYS */;
INSERT INTO `category` VALUES (2,'Unprintworthy_redirects',1607621,21,0),(3,'Computer_storage_devices',88,11,0),(7,'Unknown-importance_Animation_articles',319,21,0),(8,'Low-importance_Animation_articles',14574,21,0),(9,'Vietnam_stubs',308,10,0),(10,'Rivers_of_Vietnam',1
04,3,0),(12,'All_articles_with_unsourced_statements',507267,0,0),(14,'Wikipedia_articles_needing_clarification',205,205,0),(15,'Articles_needing_additional_references_from_January_2008',1062,0,0),(16,'Comedy',91,28,0),(17,'Sociolinguistics',256,30,0),(18,'Figures_of_spe
ech',159,13,0),(20,'NASCAR_teams',133,3,0),(21,'Muhammad_Ali',17,5,0),(22,'Politics_and_government_work_group_articles',260205,4,0),(23,'Wikipedia_requested_photographs_of_politicians_and_government-people',12442,1,0),(24,'Stub-Class_biography_(politics_and_government)_
articles',130710,0,0),(26,'Stub-Class_biography_articles',1031366,10,0),(27,'Unassessed_biography_articles',38302,10,0),(29,'High-importance_Animation_articles',286,21,0),(31,'AfD_debates',609,12,0),(32,'Articles_with_unsourced_statements',212,207,0),(35,'Self-published
_work',95334,1,95332),(36,'Geography',85,33,0),(37,'Images_without_source',0,0,0),(38,'Candidates_for_speedy_deletion',10,0,0),(40,'All_non-free_media',745544,3,745541),(41,'Wikipedia_requested_photographs_of_sportspeople',15603,1,0),(42,'Thirty_Years\'_War',61,11,0),(4
4,'African-American_history',44,34,1),(46,'History_of_Alabama',81,30,0),(47,'Groups_of_World_War_II',32,0,0),(48,'Congressional_Gold_Medal_recipients',429,3,6),(49,'United_States_Army_officers',3754,18,0),(50,'Tuskegee_University',15,4,0),(51,'Military_units_and_formati
ons_of_the_United_States_in_World_War_II',15,7,0),(52,'People_from_Tuskegee,_Alabama',58,2,0),(53,'Tuskegee_Airmen',175,0,0),(54,'Chinese_Methodists',11,3,0),(55,'Chinese_Protestants',33,15,0),(56,'Shel_Silverstein_songs',6,0,0),(57,'Articles_lacking_sources',269,213,0)
,(58,'All_articles_lacking_sources',115982,0,0),(59,'Radio_stations_in_Saskatchewan',41,7,0),(60,'Western_Canada_radio_station_stubs',43,3,0),(61,'Saskatchewan_stubs',98,5,0),(62,'Multi-level_marketing',7,2,0),(64,'Filipino_Wikipedians',746,3,0),(65,'Wikipedians_interes
ted_in_mapmaking',329,0,0),(66,'Wikipedians_interested_in_maps',1266,1,0),(67,'Wikipedians_who_listen_to_world_music',299,4,0),(68,'Wikipedians_interested_in_architecture',1031,2,0),(69,'Wikipedians_interested_in_art',1023,17,0),(70,'Wikipedian_ballroom_dancers',84,0,0)
,(71,'Wikipedian_dancers',293,5,0),(72,'Brasília',16,11,0),(74,'Sligo',0,0,0),(75,'Puerto_Rican_people',33,31,0),(77,'WikiProject_Canadian_communities_articles',2,2,0),(78,'B-Class_Canadian_communities_articles',145,0,0),(79,'Mid-importance_Canadian_communities_articles
',3316,0,0),(81,'Importance_or_significance_not_asserted_pages_for_speedy_deletion',0,0,0),(82,'The_Taming_of_the_Shrew',9,1,0),(83,'Edwards_County,_Kansas',11,4,0),(86,'Unknown-importance_Olympics_articles',13688,0,0),(87,'WikiProject_Olympics_articles',189835,3,0),(88
,'Stub-Class_Canadian_communities_articles',8027,0,0),(89,'Articles_lacking_sources_from_December_2007',229,0,0),(90,'Kingston,_Jamaica',28,9,0),(94,'Wikipedians_interested_in_Japanese_mythology',41,1,0),(98,'1978',44,37,0),(102,'Musicians_work_group_articles',168264,4,
0),(103,'Wikipedia_requested_photographs_of_musicians',12605,0,0),(104,'Start-Class_biography_(musicians)_articles',71977,0,0),(106,'Musicians_work_group_articles_needing_infoboxes',1003,0,0),(107,'Biography_articles_without_infoboxes',20015,10,0),(108,'Start-Class_biog
raphy_articles',718276,10,0),(109,'Punk_song_stubs',71,0,0),(111,'Planetary_nebulae',125,1,0),(112,'Methane',53,3,0),(113,'Economy_of_Russia',96,23,0),(114,'Climate_of_Texas',7,0,0),(115,'Transport_in_Burma',0,0,0),(116,'Townships_in_Kansas',817,2,0),(117,'Kansas_geogra
phy_stubs',320,10,0),(118,'Series_of_children\'s_books',655,84,0),(121,'List-Class_Animation_articles',1340,8,0),(122,'Start-Class_Animation_articles',7172,8,0),(123,'Wikipedia_references_cleanup',182,179,0),(125,'Theorists',21,17,0),(126,'National_lower_houses',117,26,
0),(127,'Spoken_articles',2024,0,0),(128,'United_States_House_of_Representatives',58,11,0),(129,'People_by_nationality',246,246,0);
)";

std::string categorylinks_head = R"(
-- MySQL dump 10.19  Distrib 10.3.38-MariaDB, for debian-linux-gnu (x86_64)
--
-- Host: db1206    Database: enwiki
-- ------------------------------------------------------
-- Server version       10.4.26-MariaDB-log

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8mb4 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

--
-- Table structure for table `categorylinks`
--

DROP TABLE IF EXISTS `categorylinks`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `categorylinks` (
  `cl_from` int(8) unsigned NOT NULL DEFAULT 0,
  `cl_to` varbinary(255) NOT NULL DEFAULT '',
  `cl_sortkey` varbinary(230) NOT NULL DEFAULT '',
  `cl_timestamp` timestamp NOT NULL DEFAULT current_timestamp() ON UPDATE current_timestamp(),
  `cl_sortkey_prefix` varbinary(255) NOT NULL DEFAULT '',
  `cl_collation` varbinary(32) NOT NULL DEFAULT '',
  `cl_type` enum('page','subcat','file') NOT NULL DEFAULT 'page',
  PRIMARY KEY (`cl_from`,`cl_to`),
  KEY `cl_timestamp` (`cl_to`,`cl_timestamp`),
  KEY `cl_sortkey` (`cl_to`,`cl_type`,`cl_sortkey`,`cl_from`),
  KEY `cl_collation_ext` (`cl_collation`,`cl_to`,`cl_type`,`cl_from`)
) ENGINE=InnoDB DEFAULT CHARSET=binary ROW_FORMAT=COMPRESSED;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `categorylinks`
--

/*!40000 ALTER TABLE `categorylinks` DISABLE KEYS */;
INSERT INTO `categorylinks` VALUES (10,'Redirects_from_moves','*..2NN:,@2.FBHRP:D6^A^W^Aܽ<DC>^L','2014-10-26 04:50:23','','uca-default-u-kn','page'),(10,'Re
directs_with_old_history','*..2NN:,@2.FBHRP:D6^A^W^Aܽ<DC>^L','2010-08-26 22:38:36','','uca-default-u-kn','page'),(10,'Unprintworthy_redirects','*..2NN:,@2.F
BHRP:D6^A^W^Aܽ<DC>^L','2010-08-26 22:38:36','','uca-default-u-kn','page'),(12,'Anarchism','^D^C^F*D*L.8:NB^A^O^A<C4><DC>^L','2020-01-23 13:27:44',' ','uca-d
efault-u-kn','page'),(12,'Anti-capitalism','*D*L.8:NB^A\r^A<DC>^L','2020-01-23 13:27:44','','uca-default-u-kn','page'),(12,'Anti-fascism','*D*L.8:NB^A\r^A
<DC>^L','2020-01-23 13:27:44','','uca-default-u-kn','page'),(12,'Articles_containing_French-language_text','*D*L.8:NB^A\r^A<DC>^L','2020-01-23 13:27:44',''
,'uca-default-u-kn','page'),(12,'Articles_prone_to_spam_from_November_2014','*D*L.8:NB^A\r^A<DC>^L','2020-01-23 13:27:44','','uca-default-u-kn','page'),(12
,'Articles_with_BNE_identifiers','*D*L.8:NB^A\r^A<DC>^L','2021-08-29 20:33:32','','uca-default-u-kn','page'),(12,'Articles_with_BNF_identifiers','*D*L.8:NB
^A\r^A<DC>^L','2021-08-29 20:33:32','','uca-default-u-kn','page'),(12,'Articles_with_BNFdata_identifiers','*D*L.8:NB^A\r^A<DC>^L','2023-03-13 11:56:39','',
'uca-default-u-kn','page'),(12,'Articles_with_Curlie_links','*D*L.8:NB^A\r^A<DC>^L','2022-12-10 09:46:33','','uca-default-u-kn','page'),(12,'Articles_with_
EMU_identifiers','*D*L.8:NB^A\r^A<DC>^L','2021-08-29 20:33:32','','uca-default-u-kn','page'),(12,'Articles_with_FAST_identifiers','*D*L.8:NB^A\r^A<DC>^L','
2022-10-11 09:10:41','','uca-default-u-kn','page'),(12,'Articles_with_GND_identifiers','*D*L.8:NB^A\r^A<DC>^L','2021-08-29 20:33:32','','uca-default-u-kn',
'page'),(12,'Articles_with_HDS_identifiers','*D*L.8:NB^A\r^A<DC>^L','2021-08-29 20:33:32','','uca-default-u-kn','page'),(12,'Articles_with_J9U_identifiers'
,'*D*L.8:NB^A\r^A<DC>^L','2022-02-21 19:07:42','','uca-default-u-kn','page');
    )";


TEST(SQLParser, Simple) {
    std::istringstream stream(categorylinks_head);
    SQLParser iter{stream, "categorylinks"};
    const auto row = iter.next();
    ASSERT_TRUE(row.has_value());
    ASSERT_EQ(row->at(0), "10");
    ASSERT_EQ(row->at(1), "Redirects_from_moves");
    ASSERT_EQ(row->at(2), "*..2NN:,@2.FBHRP:D6^A^W^Aܽ<DC>^L");
    ASSERT_EQ(row->at(3), "2014-10-26 04:50:23");
    ASSERT_EQ(row->at(4), "");
    ASSERT_EQ(row->at(5), "uca-default-u-kn");
    ASSERT_EQ(row->at(6), "page");
}

TEST(SQLParser, CategoryLinksParser) {
    std::istringstream stream(categorylinks_head);
    CategoryLinksParser iter{stream};
    auto row = iter.next();
    ASSERT_TRUE(row.has_value());
    ASSERT_EQ(row->page_id, 10);
    ASSERT_EQ(row->category_name, "Redirects_from_moves");
    ASSERT_EQ(row->page_type, CategoryLinkType::PAGE);
    row = iter.next();
    ASSERT_TRUE(row.has_value());
    ASSERT_EQ(row->page_id, 10);
    ASSERT_EQ(row->category_name, "Redirects_with_old_history");
    ASSERT_EQ(row->page_type, CategoryLinkType::PAGE);
    row = iter.next();
    ASSERT_TRUE(row.has_value());
    ASSERT_EQ(row->page_id, 10);
    ASSERT_EQ(row->category_name, "Unprintworthy_redirects");
    ASSERT_EQ(row->page_type, CategoryLinkType::PAGE);
    row = iter.next();
    ASSERT_TRUE(row.has_value());
    ASSERT_EQ(row->page_id, 12);
    ASSERT_EQ(row->category_name, "Anarchism");
    ASSERT_EQ(row->page_type, CategoryLinkType::PAGE);
    row = iter.next();
    ASSERT_TRUE(row.has_value());
    ASSERT_EQ(row->page_id, 12);
    ASSERT_EQ(row->category_name, "Anti-capitalism");
    ASSERT_EQ(row->page_type, CategoryLinkType::PAGE);
    row = iter.next();
    ASSERT_TRUE(row.has_value());
    ASSERT_EQ(row->page_id, 12);
    ASSERT_EQ(row->category_name, "Anti-fascism");
    ASSERT_EQ(row->page_type, CategoryLinkType::PAGE);
    row = iter.next();
    ASSERT_TRUE(row.has_value());
    ASSERT_EQ(row->page_id, 12);
    ASSERT_EQ(row->category_name, "Articles_containing_French-language_text");
    ASSERT_EQ(row->page_type, CategoryLinkType::PAGE);
    row = iter.next();
    ASSERT_TRUE(row.has_value());
    ASSERT_EQ(row->page_id, 12);
    ASSERT_EQ(row->category_name, "Articles_prone_to_spam_from_November_2014");
    ASSERT_EQ(row->page_type, CategoryLinkType::PAGE);
    row = iter.next();
    ASSERT_TRUE(row.has_value());
    ASSERT_EQ(row->page_id, 12);
    ASSERT_EQ(row->category_name, "Articles_with_BNE_identifiers");
    ASSERT_EQ(row->page_type, CategoryLinkType::PAGE);
    row = iter.next();
    ASSERT_TRUE(row.has_value());
    ASSERT_EQ(row->page_id, 12);
    ASSERT_EQ(row->category_name, "Articles_with_BNF_identifiers");
    ASSERT_EQ(row->page_type, CategoryLinkType::PAGE);
    row = iter.next();
    ASSERT_TRUE(row.has_value());
    ASSERT_EQ(row->page_id, 12);
    ASSERT_EQ(row->category_name, "Articles_with_BNFdata_identifiers");
    ASSERT_EQ(row->page_type, CategoryLinkType::PAGE);
    row = iter.next();
    ASSERT_TRUE(row.has_value());
    ASSERT_EQ(row->page_id, 12);
    ASSERT_EQ(row->category_name, "Articles_with_Curlie_links");
    ASSERT_EQ(row->page_type, CategoryLinkType::PAGE);
    row = iter.next();
    ASSERT_TRUE(row.has_value());
}

TEST(SQLParser, CategoryTable) {
    std::istringstream stream{category_table_head};
    CategoryParser iter{stream};
    auto row = iter.next();
    ASSERT_TRUE(row.has_value());
    ASSERT_EQ(row->category_id, 2);
    ASSERT_EQ(row->category_name, "Unprintworthy_redirects");
    ASSERT_EQ(row->page_count, 1607621);
    ASSERT_EQ(row->subcategory_count, 21);
    row = iter.next();
    ASSERT_TRUE(row.has_value());
    ASSERT_EQ(row->category_id, 3);
    ASSERT_EQ(row->category_name, "Computer_storage_devices");
    ASSERT_EQ(row->page_count, 88);
    ASSERT_EQ(row->subcategory_count, 11);
    row = iter.next();
    ASSERT_TRUE(row.has_value());
    ASSERT_EQ(row->category_id, 7);
    ASSERT_EQ(row->category_name, "Unknown-importance_Animation_articles");
    ASSERT_EQ(row->page_count, 319);
    ASSERT_EQ(row->subcategory_count, 21);
    row = iter.next();
    ASSERT_TRUE(row.has_value());
    ASSERT_EQ(row->category_id, 8);
    ASSERT_EQ(row->category_name, "Low-importance_Animation_articles");
    ASSERT_EQ(row->page_count, 14574);
    ASSERT_EQ(row->subcategory_count, 21);
    row = iter.next();
    ASSERT_TRUE(row.has_value());
    ASSERT_EQ(row->category_id, 9);
    ASSERT_EQ(row->category_name, "Vietnam_stubs");
    ASSERT_EQ(row->page_count, 308);
    ASSERT_EQ(row->subcategory_count, 10);
    row = iter.next();
    ASSERT_TRUE(row.has_value());
    ASSERT_EQ(row->category_id, 10);
    ASSERT_EQ(row->category_name, "Rivers_of_Vietnam");
    ASSERT_EQ(row->page_count, 104);
    ASSERT_EQ(row->subcategory_count, 3);
    row = iter.next();
    ASSERT_TRUE(row.has_value());
    ASSERT_EQ(row->category_id, 12);
    ASSERT_EQ(row->category_name, "All_articles_with_unsourced_statements");
    ASSERT_EQ(row->page_count, 507267);
    ASSERT_EQ(row->subcategory_count, 0);
    row = iter.next();
    ASSERT_TRUE(row.has_value());
    ASSERT_EQ(row->category_id, 14);
    ASSERT_EQ(row->category_name, "Wikipedia_articles_needing_clarification");
    ASSERT_EQ(row->page_count, 205);
    ASSERT_EQ(row->subcategory_count, 205);
    row = iter.next();
    ASSERT_TRUE(row.has_value());
    ASSERT_EQ(row->category_id, 15);
    ASSERT_EQ(row->category_name, "Articles_needing_additional_references_from_January_2008");
    ASSERT_EQ(row->page_count, 1062);

    ASSERT_EQ(row->subcategory_count, 0);
    row = iter.next();
    ASSERT_TRUE(row.has_value());
    ASSERT_EQ(row->category_id, 16);
    ASSERT_EQ(row->category_name, "Comedy");
    ASSERT_EQ(row->page_count, 91);
    ASSERT_EQ(row->subcategory_count, 28);
    row = iter.next();
    ASSERT_TRUE(row.has_value());
    ASSERT_EQ(row->category_id, 17);
    ASSERT_EQ(row->category_name, "Sociolinguistics");
    ASSERT_EQ(row->page_count, 256);
    ASSERT_EQ(row->subcategory_count, 30);
    row = iter.next();
    ASSERT_TRUE(row.has_value());
    ASSERT_EQ(row->category_id, 18);

    ASSERT_EQ(row->category_name, "Figures_of_speech");
    ASSERT_EQ(row->page_count, 159);
    ASSERT_EQ(row->subcategory_count, 13);

    row = iter.next();
    ASSERT_TRUE(row.has_value());
    ASSERT_EQ(row->category_id, 20);
    ASSERT_EQ(row->category_name, "NASCAR_teams");
    ASSERT_EQ(row->page_count, 133);
    ASSERT_EQ(row->subcategory_count, 3);
    row = iter.next();

    ASSERT_TRUE(row.has_value());
    ASSERT_EQ(row->category_id, 21);
    ASSERT_EQ(row->category_name, "Muhammad_Ali");
    ASSERT_EQ(row->page_count, 17);
    ASSERT_EQ(row->subcategory_count, 5);
}

TEST(BoundedStringRing, TestBoundedStringRingSimple) {
    BoundedStringRing ring{5};
    ring.push_back('a');
    ring.push_back('b');
    ring.push_back('c');
    ring.push_back('d');
    ring.push_back('e');
    ASSERT_TRUE(ring == "abcde");
    ring.push_back('f');
    ASSERT_TRUE(ring == "bcdef");
    ring.push_back('q');
    ASSERT_TRUE(ring == "cdefq");
}

TEST(BoundedStringRing, TestBoundedStringRingOverflowWithSameCharacter) {
    BoundedStringRing ring{5};
    ring.push_back('a');
    ring.push_back('a');
    ring.push_back('a');
    ring.push_back('a');
    ring.push_back('a');
    ASSERT_TRUE(ring == "aaaaa");
    ring.push_back('a');
    ASSERT_TRUE(ring == "aaaaa");
    ring.push_back('b');
    ASSERT_TRUE(ring == "aaaab");
    ring.push_back('a');
    ASSERT_TRUE(ring == "aaaba");
}

TEST(BoundedStringRing, TestBoundedStringRingOverflow) {
    BoundedStringRing ring{5};
    ring.push_back('a');
    ring.push_back('b');
    ring.push_back('c');
    ring.push_back('d');
    ring.push_back('e');
    ASSERT_TRUE(ring == "abcde");
    ring.push_back('f');
    ASSERT_EQ("bcdef", ring.str());
    ring.push_back('g');
    ASSERT_TRUE(ring == "cdefg");
    ring.push_back('h');
    ASSERT_TRUE(ring == "defgh");
    ring.push_back('i');
    ASSERT_TRUE(ring == "efghi");
    ring.push_back('j');
    ASSERT_TRUE(ring == "fghij");
    ring.push_back('k');
    ASSERT_TRUE(ring == "ghijk");
    ring.push_back('l');
    ASSERT_TRUE(ring == "hijkl");
    ring.push_back('m');
    ASSERT_TRUE(ring == "ijklm");
}

auto main(int argc, char** argv) -> int {
    spdlog::set_level(spdlog::level::debug);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}