/*
** Copyright (c) 2003, 2006 Actian Corporation. All Rights Reserved.
*/

using System;

namespace Ingres.Utility
{
	/*
	** Name: ivararray.cs
	**
	** Description:
	**	Defines interface for variable sized arrays.
	**
	**  Interfaces
	**
	**	VarArray
	**
	** History:
	**	 5-Sep-03 (gordy)
	**	    Created.
	**	 9-Dec-03 (thoda04)
	**	    Ported to .NET.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	*/


	/*
	** Name: IVarArray
	**
	** Description:
	**	Interface which provides access to variable sized arrays.
	**
	**	Arrays are variable length with a current length and capacity.
	**	Arrays may also have an optional maximum length (size limit).  
	**
	**	Initially, arrays have zero length, an implementation determined
	**	capacity, and no size limit.  The length of the array can grow
	**	until it reaches the size limit (if set).  Array capacity is 
	**	automatically extended to accomodate growth requests.  A miminum 
	**	capacity may be set to minimize costly changes in the capacity 
	**	of the array.
	**
	**  Public Methods:
	**
	**	capacity	Determine capacity of the array.
	**	ensureCapacity	Set minimum capacity of the array.
	**	limit		Set or determine maximum capacity of the array.
	**	length		Determine the current length of the array.
	**	clear		Set array length to zero.
	**
	** History:
	**	 5-Sep-03 (gordy)
	**	    Created.
	*/

	internal interface
		IVarArray
	{


		/*
		** Name: capacity
		**
		** Description:
		**	Returns the current capacity of the array.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	int	Current capacity.
		**
		** History:
		**	 5-Sep-03 (gordy)
		**	    Created.
		*/

		int capacity();


		/*
		** Name: ensureCapacity
		**
		** Description:
		**	Current capacity of the array is increased if less than the 
		**	requested capacity.  Values, including negative values, less
		**	than the current capacity are ignored.
		**
		** Input:
		**	capacity	Minimum capacity.
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
		*/

		void ensureCapacity( int capacity );


		/*
		** Name: limit
		**
		** Description:
		**	Determine the current maximum size of the array.
		**	If a maximum size has not been set, -1 is returned.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	int	Maximum size or -1.
		**
		** History:
		**	 5-Sep-03 (gordy)
		**	    Created.
		*/

		int valuelimit();


		/*
		** Name: limit
		**
		** Description:
		**	Set the maximum size of the array.  A negative size 
		**	removes any prior size limit.  The array wll be
		**	truncated if the current length is greater than the
		**	new maximum size.  Array capacity is not affected.
		**
		** Input:
		**	size	Maximum size.
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
		*/

		void valuelimit( int size );


		/*
		** Name: limit
		**
		** Description:
		**	Set the maximum size of the array and optionally ensure
		**	the mimimum capacity.  A negative size removes any prior 
		**	size limit.  The array will be truncated if the current 
		**	length is greater than the new maximum size.  Array
		**	capacity will not decrease but may increase.
		**
		**	Note that valuelimit(n, true) is equivilent to valuelimit(n)
		**	followed by ensureCapacity(n) and valuelimit(n, false) is
		**	the same as valuelimit(n).
		**
		** Input:
		**	size	Maximum size.
		**	ensure	True if capacity should be ensured as max size.
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
		*/

		void valuelimit( int size, bool ensure );


		/*
		** Name: length
		**
		** Description:
		**	Returns the current length of the array.
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
		*/

		int valuelength();


		/*
		** Name: clear
		**
		** Description:
		**	Sets the length of the array to zero.
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
		*/

		void clear();


	} // interface IVarArray

}  // namespace
