-- Seed accounts for integration testing. MySQL runs this once, when the
-- database is first initialised, i.e. before bnetd connects.
--
--   admin              password "adminpass"   command groups 1-8
--   player1 - player8  password "playerpass"
--   player9            password "playerpass"  locked
--
-- Passwords are stored as the bnet hash of the lower case password; generate
-- one with the bnpass tool. Only the columns needed here are created: bnetd
-- adds the rest of the table, the other tables and the default account
-- (uid 0) from sql_DB_layout.conf when it starts.

CREATE TABLE pvpgn_BNET (
    uid                 int NOT NULL PRIMARY KEY DEFAULT '0',
    acct_username       varchar(32)  DEFAULT NULL,
    username            varchar(32)  DEFAULT NULL,
    acct_userid         int          DEFAULT NULL,
    acct_passhash1      varchar(128) DEFAULT NULL,
    auth_lockk          varchar(16)  DEFAULT '0',
    auth_command_groups varchar(128) DEFAULT '1'
);

-- auth_command_groups is a bit mask: group N is bit N-1, so 255 = groups 1-8
INSERT INTO pvpgn_BNET
    (uid, acct_username, username, acct_userid, acct_passhash1, auth_lockk, auth_command_groups)
VALUES
    (1,  'admin',   'admin',   1,  '473cf361af0ea97f441728940760410763063913', '0',    '255'),
    (2,  'player1', 'player1', 2,  '0167717a41c7f9a3dd97f6ce8348175087d9221a', '0',    '1'),
    (3,  'player2', 'player2', 3,  '0167717a41c7f9a3dd97f6ce8348175087d9221a', '0',    '1'),
    (4,  'player3', 'player3', 4,  '0167717a41c7f9a3dd97f6ce8348175087d9221a', '0',    '1'),
    (5,  'player4', 'player4', 5,  '0167717a41c7f9a3dd97f6ce8348175087d9221a', '0',    '1'),
    (6,  'player5', 'player5', 6,  '0167717a41c7f9a3dd97f6ce8348175087d9221a', '0',    '1'),
    (7,  'player6', 'player6', 7,  '0167717a41c7f9a3dd97f6ce8348175087d9221a', '0',    '1'),
    (8,  'player7', 'player7', 8,  '0167717a41c7f9a3dd97f6ce8348175087d9221a', '0',    '1'),
    (9,  'player8', 'player8', 9,  '0167717a41c7f9a3dd97f6ce8348175087d9221a', '0',    '1'),
    (10, 'player9', 'player9', 10, '0167717a41c7f9a3dd97f6ce8348175087d9221a', 'true', '1');
