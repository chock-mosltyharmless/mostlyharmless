/**
 * Package for all kinds of utilities that can be used independent
 * of a game (like mathematics or parsing)
 */
package de.badlegrunners.braveryrun.util;

import java.util.Scanner;

/**
 * Used to simplify scanning of text files with checking whether
 * the text file is syntactically correct.
 * 
 * @author chock
 */
public class DataScanner {
	/**
	 * The scanner object that is used to iterate through
	 * the input text.
	 */
	private Scanner scanner;
	
	/**
	 * Initialization of the data scanner. The scanner object
	 * must point to the right part where scanning is done.
	 * The scanner object is modified during scanning (input
	 * data is eaten up).
	 * 
	 * @param scanner
	 */
	public DataScanner(Scanner scanner) {
		this.scanner = scanner;
	}
	
	/**
	 * Read the next item and check that it is first identified
	 * by the given identifier. An Exception is thrown if the
	 * next input token is not the identifier.
	 * The scanner moves beyond the next two tokens.
	 * 
	 * @param identifier The string that shall be the next input token.
	 * @param exceptionHead Header to put in front of the thrown
	 * 		  exception if an error occured
	 * @return The integer from the text file.
	 * @throws Exception
	 */
	public int getNextInt(String identifier, String exceptionHead)
		throws Exception {
		checkToken(identifier, exceptionHead);
		return scanner.nextInt();
	}
	
	/**
	 * Check whether the next token is the specified identifier.
	 * An exception is thrown if there is no fit. See also
	 * getNextInt()
	 * 
	 * @param identifier The identifier to check against the token
	 * @param exceptionHead The header to put before the Exception
	 * @throws Exception If there was an error.
	 */
	public void checkToken(String identifier, String exceptionHead) 
		throws Exception {
		String token = scanner.next();
		if (!token.equals(identifier)) {
			throw new Exception(exceptionHead + ": Token " +
					token + " was found while " + identifier +
					" was expected.");
		}
	}
}
