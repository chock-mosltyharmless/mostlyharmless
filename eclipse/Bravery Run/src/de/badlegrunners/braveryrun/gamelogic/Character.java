/**
 * Package for all the game logic
 */
package de.badlegrunners.braveryrun.gamelogic;

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
	protected final int maximumHP = 100;
	
	/**
	 * Size of the character
	 * This influences what stuff the character can take, but
	 * larger size also makes for an easier target.
	 */
	protected int siz; 
	
	/**
	 * Speed of the character.
	 * Determines how fast the character can react again. Also
	 * influences the avoid skill and some attack skills.
	 */
	protected int spd;
	
	/**
	 * Strength of the character.
	 * This is mostly used for physical weapons, but also for some
	 * magic.
	 */
	protected int str;
	
	/**
	 * Agility of the character.
	 * This is used for some of the attack types and also the
	 * avoid possibility.
	 */
	protected int agi;
	
	/**
	 * Wisdom of the character.
	 * Used for magic attack and magic defense.
	 */
	protected int wis;
	
	/**
	 * Current hit points of the character.
	 * Maximum hit points is always 100.
	 */
	protected int hp;
	
	/**
	 * Get the size of the character
	 * @return The size of the character
	 */
	public int getSiz() {return siz;}
	
	/**
	 * Get the speed of the character
	 * @return The speed of the character
	 */
	public int getSpd() {return spd;}
	
	/**
	 * Get the strength of the character
	 * @return The strength of the character
	 */
	public int getStr() {return str;}
	
	/**
	 * Get the agility of the character
	 * @return The agility of the character
	 */
	public int getAgi() {return agi;}
	
	/**
	 * Get the wisdom of the character
	 * @return The wisdom of the character
	 */
	public int getWis() {return wis;}
	
	/**
	 * Get the current hp of the character
	 * @return The hp of the character
	 */
	public int getHP() {return hp;}
	
	/**
	 * Reset the hit points to the maximum value. This should be
	 * done before each battle starts.
	 */
	public void resetHP() {
		hp = maximumHP;
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
