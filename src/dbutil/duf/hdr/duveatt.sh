/* Description of table iiattribute from database iidbdb */
  EXEC SQL DECLARE iiattribute TABLE
	(attrelid	integer not null,
	 attrelidx	integer not null,
	 attid	smallint not null,
	 attxtra	smallint not null,
	 attoff	integer not null,
	 attfrml	integer not null,
	 attkdom	smallint not null,
	 attflag	smallint not null,
	 attdefid1	integer not null,
	 attdefid2	integer not null,
	 attintl_id	smallint not null,
	 attver_added	smallint not null,
	 attver_dropped	smallint not null,
	 attval_from	smallint not null,
	 attfrmt	smallint not null,
	 attfrmp	smallint not null,
	 attver_altcol	smallint not null,
	 attcollid	smallint not null,
	 attsrid	integer not null,
	 attgeomtype	smallint not null,
	 attencflags	smallint not null,
	 attencwid	integer not null,
	 attfree	byte(4) not null,
	 attname	char(256) not null);

  struct att_tbl_ {
	i4	attrelid;
	i4	attrelidx;
	i2	attid;
	i2	attxtra;
	i4	attoff;
	i4	attfrml;
	i2	attkdom;
	i2	attflag;
	i4	attdefid1;
	i4	attdefid2;
	i2	attintl_id;
	i2	attver_added;
	i2	attver_dropped;
	i2	attval_from;
	i2	attfrmt;
	i2	attfrmp;
	i2	attver_altcol;
	i2	attcollid;
	i4	attsrid;
	i2	attgeomtype;
	i2	attencflags;
	i4	attencwid;
	char	attfree[4];
	char	attname[257];
  } att_tbl;
