BEGIN TRANSACTION;
CREATE TABLE IF NOT EXISTS "Notes" (
	"ID"	INTEGER NOT NULL UNIQUE,
	"Text"	TEXT,
	"Color"	TEXT,
	"GroupID"	INTEGER,
	"Name"	TEXT,
	FOREIGN KEY("GroupID") REFERENCES "Groups"("ID"),
	PRIMARY KEY("ID" AUTOINCREMENT)
);
CREATE TABLE IF NOT EXISTS "Groups" (
	"ID"	INTEGER NOT NULL UNIQUE,
	"Name"	TEXT,
	"Password"	TEXT,
	PRIMARY KEY("ID" AUTOINCREMENT)
);
INSERT INTO Groups(Name) VALUES('Installationsnoter');
INSERT INTO Notes(Name, Text, GroupID)
SELECT
    'Min første note' AS "Name",
    'Denne note er oprettet automatisk under installationen.\nGod fornøjelse med værktøjet.' AS "Text",
    (SELECT MAX(ID) FROM Groups) AS GroupID;
COMMIT;
