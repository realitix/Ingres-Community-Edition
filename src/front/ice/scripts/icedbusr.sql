/*
**	Copyright (c) 2004 Actian Corporation
*/
/*
**  Create a default db user
*/
create user icedbuser with
  nogroup,
  privileges=(createdb),
  default_privileges=(createdb),
  noexpire_date,
  noprofile,
  nosecurity_audit;
\p\g
