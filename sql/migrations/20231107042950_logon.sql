DROP PROCEDURE IF EXISTS add_migration;
delimiter ??
CREATE PROCEDURE `add_migration`()
BEGIN
DECLARE v INT DEFAULT 1;
SET v = (SELECT COUNT(*) FROM `migrations` WHERE `id`='20231107042950');
IF v=0 THEN
INSERT INTO `migrations` VALUES ('20231107042950');
-- Add your query below.


DROP TABLE IF EXISTS `allowed_clients`;
CREATE TABLE IF NOT EXISTS `allowed_clients` (
  `major_version` tinyint(3) unsigned NOT NULL,
  `minor_version` tinyint(3) unsigned NOT NULL,
  `bugfix_version` tinyint(3) unsigned NOT NULL,
  `hotfix_version` char(1) COLLATE latin1_bin NOT NULL,
  `build` mediumint(8) unsigned NOT NULL,
  `os` char(50) COLLATE latin1_bin NOT NULL,
  `auth_seed` varchar(40) COLLATE latin1_bin NOT NULL DEFAULT ''
) ENGINE=InnoDB DEFAULT CHARSET=latin1 COLLATE=latin1_bin;

INSERT INTO `allowed_clients` (`major_version`, `minor_version`, `bugfix_version`, `hotfix_version`, `build`, `os`, `auth_seed`) VALUES
(1, 14, 0, '', 40618, 'Wn64', '1278EB34F243ED7898D614C0E278EAC0'),
(1, 14, 0, '', 40618, 'Mc64', '7528AB80D693E149907757BC9540A6A6'),
(1, 14, 1, '', 41794, 'Wn64', '91D3C1D62CD20FCCD4D0A71E051CE7CA'),
(1, 14, 2, '', 42597, 'Wn64', '2C76A6CDD32F651E940B5F682D8E15CE'),
(1, 14, 2, '', 42597, 'MacA', '3B31A4F4C25382131A8FB95A1317412B');


-- End of migration.
END IF;
END??
delimiter ; 
CALL add_migration();
DROP PROCEDURE IF EXISTS add_migration;
