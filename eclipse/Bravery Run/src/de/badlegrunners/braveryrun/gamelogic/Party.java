package de.badlegrunners.braveryrun.gamelogic;

import de.badlegrunners.braveryrun.gamelogic.Character;
import de.badlegrunners.braveryrun.util.DataScanner;

import java.util.Scanner;

/**
 * A party consists of three Characters. A party can either be 
 * controlled by the player or by the computer.
 * 
 * @author chock
 */
public class Party {
	/**
	 * Number of members in the party
	 */
	protected final int numMembers = 3;
	
	/**
	 * The party members, ordered.
	 */
	protected final Character member[];
	
	/**
	 * @return number of members in the party.
	 */
	public int getNumMembers() {
		return numMembers;
	}
	
	/**
	 * Retrieve the character with the given index. The index
	 * must be in the range 0..(getNumMembers()-1)
	 * @param index Index of the member to retreive
	 * @return The Character with the given member index
	 */
	public Character getMember(int index) {
		return member[index];
	}
	
	/**
	 * Raw constructor (generates a random party?
	 */
	public Party() {
		member = new Character[numMembers];
		for (int i = 0; i < numMembers; i++) {
			member[i] = new Character();
		}
	}
	
	/**
	 * Load a party from a text-based dataset using a Scanner object.
	 * The scanner must point to where the party begins inside
	 * the open bracket.
	 * 
	 * @param text The input text data set
	 * @param version The version of the dataset
	 * @throws Exception If anything goes wrong during loading
	 */
	public Party(Scanner text, int version) throws Exception {
		if (version != 1) {
			throw new Exception("Party: Wrong version: " + version);
		}
		
		DataScanner datScan = new DataScanner(text);
		
		member = new Character[numMembers];
		for (int i = 0; i < numMembers; i++) {
			datScan.checkToken("Character", "Party");
			datScan.checkToken("{", "Party");
			member[i] = new Character(text, version);
			datScan.checkToken("}", "Party");
		}
	}
}
