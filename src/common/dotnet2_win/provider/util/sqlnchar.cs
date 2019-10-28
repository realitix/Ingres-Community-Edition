/*
** Copyright (c) 2003, 2006 Actian Corporation. All Rights Reserved.
*/

using System;
using System.IO;
using System.Text;

namespace Ingres.Utility
{

	/*
	** Name: sqlnchar.cs
	**
	** Description:
	**	Defines class which represents an SQL NChar value.
	**
	**  Classes:
	**
	**	SqlNChar	An SQL NChar value.
	**
	** History:
	**	 5-Sep-03 (gordy)
	**	    Created.
	**	 9-Dec-03 (thoda04)
	**	    Ported to .NET.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	**	 1-Dec-03 (gordy, ported by thoda04 1-Oct-06)
	**	    Added support for parameter types/values in addition to 
	**	    existing support for columns.
	*/



	/*
	** Name: SqlNChar
	**
	** Description:
	**	Class which represents an SQL NChar value.  SQL NChar values 
	**	are fixed length UCS2 based strings.  
	**
	**	Supports conversion to boolean, byte, short, int, long, float, 
	**	double, BigDecimal, Date, Time, Timestamp, Object and streams.  
	**
	**	This class implements the CharArray interface as the means
	**	to set the string value.  The optional size limit defines
	**	the fixed length of the string and determines the capacity
	**	and length of the char array.  If the size limit is not set,
	**	the string is treated as variable length and is equivilent
	**	to a SQL NVarChar value.
	**
	**	The string value may be set by first clearing the array and 
	**	then using the put() method to set the value.  Segmented 
	**	input values are handled by successive calls to put().  An
	**	internal input length is maintained and the value is space
	**	extended to the (optional) fixed length when accessed.  The 
	**	clear() method also clears the NULL setting.
	**
	**	The data value accessor methods do not check for the NULL
	**	condition.  The base class isNull() method should first
	**	be checked prior to calling any of the accessor methods.
	**
	**  Interface Methods:
	**
	**	limit			Set or determine fixed length of the array.
	**	length			Determine the current length of the array.
	**	set			Assign a new data value.
	**	get			Copy characters out of the array.
	**
	**  Public Methods:
	**
	**	getString		Data value accessor methods
	**
	**  Private Methods:
	**
	**	extend			Check input, extend if necessary.
	**
	** History:
	**	 5-Sep-03 (gordy)
	**	    Created.
	**	 1-Dec-03 (gordy)
	**	    Support for variable length strings incorporated by
	**	    using SqlNVarChar as base class.  Removed resulting
	**	    redundancies.
	*/

	internal class
	SqlNChar
		: SqlNVarChar, ICharArray
	{

		/*
		** Name: SqlNChar
		**
		** Description:
		**	Class constructor for variable length UCS2 strings.
		**	Data value is initially NULL.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	None.
		**
		** History:
		**	 5-Sep-03 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    Adapted to new base class.
		*/

		public
		SqlNChar() : base()
		{
		} // SqlNChar


		/*
		** Name: SqlNChar
		**
		** Description:
		**	Class constructor for fixed length UCS2 strings.
		**	Size limit determines capacity and length.  Data 
		**	value is initially NULL.
		**
		** Input:
		**	size		The fixed string length.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	None.
		**
		** History:
		**	 5-Sep-03 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    Adapted to new base class.
		*/

		public
		SqlNChar( int size ) : base( size )
		{
			ensure( size );
		} // SqlNChar


		/*
		** Name: limit
		**
		** Description:
		**	Set the fixed size of the array.  A negative size removes 
		**	any prior fixed size.  The array will be truncated if the 
		**	current length is greater than the new fixed size.
		**
		**	For fixed length strings, this method is the same as
		**	valuelimit( size, true ).
		**
		** Input:
		**	size	Fixed size.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	 5-Sep-03 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    Capacity always ensured.
		*/

		public override void
		valuelimit( int size )
		{
			base.valuelimit( size, true );
			return;
		} // limit


		/*
		** Name: limit
		**
		** Description:
		**	Set the fixed size of the array.  A negative size removes any 
		**	prior fixed size.  The array will be truncated if the current 
		**	length is greater than the new fixed size.
		**
		**	Note that for fixed length strings, the capacity is determined
		**	by the size limit and this implementation ignores the ensure
		**	parameter.
		**
		** Input:
		**	size	Fixed size.
		**	ensure	Ignored (capacity always ensured to limit).
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	 5-Sep-03 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    Capacity always ensured.
		*/

		public override void
		valuelimit( int size, bool ensure )
		{
			base.valuelimit( size, true );
			return;
		} // limit


		/*
		** Name: length
		**
		** Description:
		**	Returns the current length of the array.
		**
		**	Fixed length strings are space extended on access and this
		**	implementation will always return the size limit when set.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	int	Current length.
		**
		** History:
		**	 5-Sep-03 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    Return limit if set.
		*/

		public override int
		valuelength()
		{
			return ((limit >= 0) ? limit : length);
		} // length


		/*
		** Name: set
		**
		** Description:
		**	Assign a new data value as a copy of an existing 
		**	SQL data object.  If the input is NULL, a NULL 
		**	data value results.
		**
		** Input:
		**	data	The SQL data to be copied.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public void
		set( SqlNChar data )
		{
			if ( data == null  ||  data.isNull() )
			setNull();
			else
			{
			clear();
			put(data.value, 0, data.length);
			}
			return;
		} // set


		/*
		** Name: get
		**
		** Description:
		**	Returns the character at a specific position in the 
		**	array.  If position is beyond the current array length, 
		**	0 is returned.
		**
		** Input:
		**	position	Array position.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	char		Character or 0.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public override char 
		get( int position )
		{
			/*
			** Only need to extend if position is greater than current length.
			*/
			if (position >= length) extend();
			return( base.get( position ) );
		} // get


		/*
		** Name: get
		**
		** Description:
		**	Returns a copy of the array.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	char[]	Copy of the current array.
		**
		** History:
		**	 5-Sep-03 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    Extend input to fixed length.  Use base class method.
		*/

		public override char[] 
		get()
		{
			extend();
			return( base.get() );
		} // get


		/*
		** Name: get
		**
		** Description:
		**	Copy a portion of the array.  Copying starts at the position
		**	indicated.  The number of characters copied is the minimum of 
		**	the length requested and the number of characters in the array 
		**	starting at the requested position.  If position is greater 
		**	than the current length, no characters are copied.  The output 
		**	array must have sufficient space.  The actual number of 
		**	characters copied is returned.
		**
		** Input:
		**	position	Starting character to copy.
		**	length		Number of characters to copy.
		**	offset		Starting position in output array.
		**
		** Output:
		**	value		Character array to recieve output.
		**
		** Returns:
		**	int		Number of characters copied.
		**
		** History:
		**	 5-Sep-03 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    Extend input to fixed length.  Use base class method.
		*/

		public override int
		get( int position, int length, char[] value, int offset )
		{
			/*
			** Only need to extend if request is greater than current length.
			*/
			if ((position + length) > this.length) extend();
			return( base.get( position, length, value, offset ) );
		} // get


		/*
		** Name: getString
		**
		** Description:
		**	Return the current data value as a String value.
		**
		**	Note: the value returned when the data value is 
		**	NULL is not well defined.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	String	Current value converted to String.
		**
		** History:
		**	 5-Sep-03 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    Use base class method.
		*/

		public override String 
		getString() 
		{
			extend();
			return( base.getString() );
		} // getString


		/*
		** Name: getString
		**
		** Description:
		**	Return the current data value as a String value
		**	with maximum size limit.
		**
		**	Note: the value returned when the data value is 
		**	NULL is not well defined.
		**
		** Input:
		**	limit	Maximum size of result.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	String	Current value converted to String.
		**
		** History:
		**	 5-Sep-03 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    Use base class method.
		*/

		public override String 
		getString( int limit ) 
		{
			/*
			** Only need to extend if max size is greater than current length.
			*/
			if (limit > length) extend();
			return( base.getString( limit ) );
		} // getString


		/*
		** Name: getCharacterStream
		**
		** Description:
		**	Return the current data value as a character stream.
		**
		**	Note: the value returned when the data value is 
		**	NULL is not well defined.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	Reader	Current value converted to character stream.
		**
		** History:
		**	 5-Sep-03 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    Use base class method.
		*/

		public override Reader 
		getCharacterStream() 
		{ 
			extend();
			return( base.getCharacterStream() );
		} // getCharacterStream


		/*
		** Name: extend
		**
		** Description:
		**	Check input length and extend with space characters if necessary.
		**	This routine may be called multiple times with no side-effects.  
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	 5-Sep-03 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    Limit now optional.
		*/

		public void	// access for SqlLongNChar
		extend()
		{
			for (; length < limit; length++)
				value[length] = ' ';
			return;
		} // extend


	} // class SqlNChar

}  // namespace
