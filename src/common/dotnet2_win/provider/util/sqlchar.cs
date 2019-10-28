/*
** Copyright (c) 2003, 2006 Actian Corporation. All Rights Reserved.
*/

using System;
using System.IO;
using System.Text;

namespace Ingres.Utility
{
	/*
	** Name: sqlchar.cs
	**
	** Description:
	**	Defines class which represents an SQL Char value.
	**
	**  Classes:
	**
	**	SqlChar		An SQL Char value.
	**
	** History:
	**	 5-Sep-03 (gordy)
	**	    Created.
	**	 1-Dec-03 (gordy)
	**	    Added support for parameter types/values in addition to 
	**	    existing support for columns.
	**	 9-Dec-03 (thoda04)
	**	    Ported to .NET.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	*/


	/*
	** Name: SqlChar
	**
	** Description:
	**	Class which represents an SQL Char value.  SQL Char values 
	**	are fixed length byte based strings.  A character-set is
	**	associated with the byte string for conversion to a 
	**	String.
	**
	**	Supports conversion to boolean, byte, short, int, long, float, 
	**	double, BigDecimal, Date, Time, Timestamp, Object and streams.  
	**
	**	This class implements the ByteArray interface as the means
	**	to set the string value.  The optional size limit defines
	**	the fixed length of the string and determines the capacity
	**	and length of the byte array.  If the size limit is not set,
	**	the string is treated as variable length and is equivilent
	**	to a SQL VarChar value.
	**
	**	The string value may be set by first clearing the array and 
	**	then using the put() method to set the value.  Segmented 
	**	input values are handled by successive calls to put().  An
	**	internal input length is maintained and the value is space
	**	extended to the (optional) fixed length when accessed.  The 
	**	clear() method also clears the NULL setting.
	**
	**	The data value accessor methods do not check for the NULL
	**	condition.  The super-class isNull() method should first
	**	be checked prior to calling any of the accessor methods.
	**
	**  Interface Methods:
	**
	**	limit			Set or determine fixed length of the array.
	**	length			Determine the current length of the array.
	**	set			Assign a new data value.
	**	get			Copy bytes out of the array.
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
	**	    using SqlVarChar as base class.  Removed resulting
	**	    redundancies.
	*/

	internal class
		SqlChar : SqlVarChar, IByteArray
	{


		/*
		** Name: SqlChar
		**
		** Description:
		**	Class constructor for variable length byte strings.
		**	Data value is initially NULL.
		**
		** Input:
		**	charSet		Character-set of byte strings.
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
		**	    Adapted to new super class.
		*/

		public
			SqlChar(CharSet charSet)
			: base(charSet)
		{
		} // SqlChar


		/*
		** Name: SqlChar
		**
		** Description:
		**	Class constructor for fixed length byte strings.  
		**	Size limit determines capacity and length.  Data 
		**	value is initially NULL.
		**
		** Input:
		**	charSet		Character-set of byte strings
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
		**	    Adapted to new super class.
		*/

		public
			SqlChar(CharSet charSet, int size)
				: base(charSet, size)
		{
			ensure(size);
		} // SqlChar


		/*
		** Name: limit
		**
		** Description:
		**	Set the fixed size of the array.  A negative size removes 
		**	any prior fixed size.  The array will be truncated if the 
		**	current length is greater than the new fixed size.
		**
		**	For fixed length strings, this method is the same as
		**	limit( size, true ).
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
		valuelimit(int size)
		{
			base.valuelimit(size, true);
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
		valuelimit(int size, bool do_ensure)
		{
			base.valuelimit(size, true);
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
		set(SqlChar data)
		{
			if (data == null || data.isNull())
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
		**	Returns the byte value at a specific position in the
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
		**	byte		Byte value or 0.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public override byte
		get(int position)
		{
			/*
			** Only need to extend if position is greater than current length.
			*/
			if (position >= length) extend();
			return (base.get(position));
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
		**	byte[]	Copy of the current array.
		**
		** History:
		**	 5-Sep-03 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    Extend input to fixed length.  Use super-class method.
		*/

		public override byte[] 
			get()
		{
			extend();
			return (base.get());
		} // get


		/*
		** Name: get
		**
		** Description:
		**	Copy a portion of the array.  Copying starts at the position
		**	indicated.  The number of bytes copied is the minimum of the
		**	length requested and the number of bytes in the array starting
		**	at the requested position.  If position is greater than the
		**	current length, no bytes are copied.  The output array must
		**	have sufficient space.  The actual number of bytes copied is
		**	returned.
		**
		** Input:
		**	position	Starting byte to copy.
		**	length		Number of bytes to copy.
		**	offset		Starting position in output array.
		**
		** Output:
		**	value		Byte array to receive output.
		**
		** Returns:
		**	int		Number of bytes copied.
		**
		** History:
		**	 5-Sep-03 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    Extend input to fixed length.  Use super-class method.
		*/

		public override int
		get( int position, int length, byte[] value, int offset )
		{
			/*
			** Only need to extend if request is greater than current length.
			*/
			if ( (position + length) > this.length )  extend();
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
		**	    Use super-class method.
		*/

		public override String 
			getString() 
		{
			extend();
			return (base.getString());
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
		**	    Use super-class method.
		*/

		public override String 
			getString( int limit ) 
		{
			/*
			** Only need to extend if max size is greater than current length.
			*/
			if (limit > length) extend();
			return (base.getString(limit));
		} // getString


		/*
		** Name: extend
		**
		** Description:
		**	Check input length and extend with space bytes if necessary.
		**	This routine may be called multiple times with no side-
		**	effects.  
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
		**	    Limit now optional.  Space character available in char-set.
		**	    Use ASCII space as default.
		*/

		public virtual void
			extend()
		{
			byte space;

			try { space = charSet.getSpaceChar(); }
			catch (Exception)
			{ space = 0x20; /* Use ASCII space as default */ }

			for (; length < limit; length++) value[length] = space;
			return;
		} // extend


	} // class SqlChar
}  // namespace
