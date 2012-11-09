/**
 * Package for all the game logic
 */
package de.badlegrunners.braveryrun.gamelogic;

import de.badlegrunners.braveryrun.gamelogic.Item;
import de.badlegrunners.braveryrun.util.DataScanner;

import java.util.Scanner;

/**
 * One character, either a player or an enemy.
 * This contains all the statistics and the items.
 * 
 * @author chock
 */
public class Character {
	/**
	 * The maximum number of hit points the character can have.
	 * In the current version, this is static.
	 */
	protected final int maxHP = 100;
	
	/**
	 * The maximum size a character may have. The sum of size and
	 * speed is maxSiz+1.
	 */
	protected final int maxSiz = 10;
	
	/**
	 * Size of the character
	 * This influences what stuff the character can take, but
	 * larger size also makes for an easier target.
	 * 
	 * Additionally, speed is 10-size
	 */
	protected final int siz;
	
	/** 
	 * The number of stats a character has. This excludes size,
	 * because size is not a skill... ???
	 * 
	 * The skills I am currently favoring are:
	 * - Strength for attack
	 * - Agility for other attacks and dodging
	 * - Wisdom for magic attack and defense
	 * - Constitution for physical and magical defense (less)
	 */
	public final int numStats = 4;
		
	/**
	 * The stat values, in range 1..9
	 */
	protected final int stat[];
	
	/**
	 * Current hit points of the character.
	 * Maximum hit points is always 100.
	 */
	protected int hp;
	
	/**
	 * The number of items that the character holds at all times.
	 * I have no inventory...
	 */
	protected final int numItems = 4;
	
	/**
	 * The items that the character holds. This is always
	 * numItems many. No item may be null.
	 */
	protected Item item[];
	
	/**
	 * Get the size of the character
	 * @return The size of the character
	 */
	public int getSiz() {return siz;}

	/**
	 * Get the speed of the character.
	 */
	public int getSpd() {return maxSiz-siz+1;}
	
	/**
	 * Get one stat of the character
	 * @param statID number between 0 and numStats-1
	 * @return The requested stat value
	 */
	public int getStat(int statID) {return stat[statID];}
		
	/**
	 * Get the current hp of the character
	 * @return The hp of the character
	 */
	public int getHP() {return hp;}
	
	/**
	 * simple constructor
	 */
	public Character() {
		siz = 1;
		stat = new int[numStats];
		// TODO: Create items, random stats?
	}
	
	/**
	 * Load character from a text-based file using a Scanner object.
	 * The scanner must be located at the position inside the opening
	 * bracket. The data for the character is consumed.
	 * 
	 * @param text The text-based input file
	 * @param version The version that is supported
	 * @throws Exception If anything goes wrong
	 */
	public Character(Scanner text, int version) throws Exception {
		if (version != 1) {
			throw new Exception("Character: Not supported Version:" +
							    version);
		}
		
		DataScanner datScan = new DataScanner(text);

		siz = datScan.getNextInt("siz", "Characer");
		
		stat = new int[numStats];
		datScan.checkToken("stat", "Character");
		datScan.checkToken("{", "Character");
		for (int i = 0; i < numStats; i++) {
			stat[i] = text.nextInt();
		}
		datScan.checkToken("}", "Character");
		
		item = new Item[numItems];
		for (int i = 0; i < numItems; i++) {
			datScan.checkToken("Item", "Character");
			datScan.checkToken("{", "Character");
			item[i] = new Item(text, version);
			datScan.checkToken("}", "Character");
		}
	}
	
	/**
	 * Reset the hit points to the maximum value. This should be
	 * done before each battle starts.
	 */
	public void resetHP() {
		hp = maxHP;
	}
	
	/**
	 * Check whether there was some cheating done on a player
	 * character.
	 * @return True if this character has incorrect information
	 */
	public boolean checkCheating() {
		// Check the the skills are in range
		// Check that the items may be carried by the character
		return false;
	}
}
