/**
 * Package for all the game logic
 */
package de.badlegrunners.braveryrun.gamelogic;

import de.badlegrunners.braveryrun.gamelogic.Item;
import de.badlegrunners.braveryrun.util.DataScanner;
import de.badlegrunners.braveryrun.util.Dice;
import java.util.Scanner;

/**
 * One character, either a player or an enemy.
 * This contains all the statistics and the items.
 * 
 * // TODO: Put all returned values (like getEvade) into
 * member variables that are only generated when items change.
 * This should be done for optimization
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
	 * Index of the strength stat
	 */
	public final int STRENGTH_IDX = 0;
	
	/**
	 * Index of the agility stat
	 */
	public final int AGILITY_IDX = 1;
	
	/**
	 * Index of the wisdom stat
	 */
	public final int WISDOM_IDX = 2;
	
	/**
	 * Index of the constitution stat
	 */
	public final int CONSTITUTION_IDX = 3;
	
	/**
	 * Current hit points of the character.
	 * Maximum hit points is always 100.
	 */
	protected int hp;
	
	/**
	 * Current time of the character. This time is used inside
	 * the battle in order to see whether the character is activated
	 * or which character is activated next.
	 * 
	 * The character is activated once the actionTime reaches 100.
	 */
	protected int actionTime;
	
	/**
	 * The time at which the action is performed. See
	 * actionTime.
	 */
	protected int maxActionTime = 1000;
	
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
	 * Reduces the amount of magical efficiency by means of the
	 * profanity skills of the items.
	 * 
	 * The multiplier is 100 - 2 * sum(profanity)
	 * That is, in a range 8..100
	 * 
	 * @param amount
	 * @return
	 */
	public int applyMagicMultiplier(int amount) {
		int multiplier = 100;
		for (int i = 0; i < numItems; i++) {
			multiplier -= 2 * item[i].getProfanity();
		}
		return multiplier;
	}
	
	/**
	 * Reset the hit points to the maximum value. This should be
	 * done before each battle starts.
	 */
	public void resetHP() {
		hp = maxHP;
	}
	
	/**
	 * Reset the action time of a character. This shall be done
	 * at the start of a battle. Resetting the time also has
	 * to be done once an action of the character is performed.
	 */
	public void resetActionTime() {
		actionTime = 0;
	}
	
	/**
	 * This method lets some time pass by inside the battle. The
	 * method returns whether the character skill may be activated now.
	 * 
	 * @param ticks The ticks in the internal time format
	 * @return Whether the character is activated.
	 */
	public boolean passTime(int ticks) {
		boolean isActivated = false;
		
		if (hp > 0) {
			actionTime += ticks * getSpd();
			if (actionTime >= maxActionTime) {
				actionTime = maxActionTime;
				isActivated = true;
			}
		}
		
		return isActivated;
	}
	
	/**
	 * This method returns whether the character is currently activated
	 * inside a battle. See passTime().
	 * 
	 * @return Whether the character is activated.
	 */
	public boolean isActivated() {
		return (actionTime >= maxActionTime) && (hp > 0);
	}
	
	/**
	 * Apply a physical hit on the target. This takes the defense
	 * ability of the character into account. Evade is not used and
	 * has to be calculated before calling damage.
	 * 
	 * Damage is calculated by subtracting amount from the physical
	 * defense.
	 * 
	 * @param amount The amount of physical damage to do
	 */
	public void physicalDamage(int amount) {
		int defense = getDefense();
		int hpReduction = amount - defense;
		if (hpReduction < 0) {
			hpReduction = 0;
		}
		hp -= hpReduction;
		if (hp < 0) {
			hp = 0;
		}
	}
	
	/**
	 * Apply magical damage on character. Evade is not taken into
	 * account. You have to calculate evade before applying magical
	 * damage. Additionally, magical damage is affected by the
	 * magical reduction of the character using the skill which is
	 * not taken into account here.
	 * 
	 * Magical damage is multiplied by the magical resistance as
	 * a percentage. Note that magical resistance can be below 0 or
	 * above 100. With a magical resistance above 100, you actually
	 * get healed by magical damage.
	 * 
	 * @param amount The amount of magical damage to apply
	 */
	public void magicalDamage(int amount) {
		int resistance = getResistance();
		// note that negative hpReduction is possible
		int hpReduction = (amount * (100 - resistance)) / 100;
		
		hp -= hpReduction;
		if (hp < 0) {
			hp = 0;
		}
		if (hp > maxHP) {
			hp = maxHP;
		}
	}
	
	/**
	 * Heal a character by a given amount.
	 * 
	 * @param amount The number of hit points to heal
	 */
	public void heal(int amount) {
		hp += amount;
		if (hp > maxHP) {
			hp = maxHP;
		}
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
	
	/**
	 * This method returns the probability that the character
	 * will evade physical or magical damage (and maybe some
	 * other effects as well. The formula has the following
	 * properties:
	 * - Higher agility improves evade
	 * - Higher speed improves evade
	 * - Some passive skill may reduce the evade (based on size?)
	 * - minimum evade is 0
	 * - maximum evade should be around 80%, I need to think about... 
	 * 
	 * Example:
	 *  evade = 4 * (speed + agility - sum(encumberance))
	 *  max: 4 * (9 + 9 - (-1*4)) = 4 * 22 = 88
	 *  
	 * @return The evade percentage as an integer
	 */
	public int getEvadeProb() {
		int evade = getSpd() + getStat(AGILITY_IDX);
		
		for (int i = 0; i < numItems; i++) {
			evade -= item[i].getEncumberance();
		}
		
		/* General multiplier to get into a range that is good
		 * for playing
		 */
		evade *= 4;

		if (evade < 0) evade = 0;
		
		return evade;
	}
	
	/**
	 * This method returns the defense value of the character based
	 * on his stats and equipment. The defense shall be determined
	 * by the following formula:
	 * 
	 * defense = sum(constitution*itemDefense)
	 * 
	 * @return The defense of the character
	 */
	public int getDefense() {
		int defense = 0;
		
		for (int i = 0; i < numItems; i++) {
			defense += item[i].getDefense() * stat[CONSTITUTION_IDX];
		}
		
		return defense;
	}
	
	/**
	 * This method returns the resistance of the character as a
	 * percentage. The resistance may be below 0 or above 100.
	 * 
	 * The resistance is basically a combination of wisdom and
	 * constitution, but it is reduced by magical reduction of
	 * the armor.
	 * 
	 * The formula is:
	 * resistance = 10 * (wisdom + constitution) - 5 * sum(profanity)
	 * 
	 * @return Resistance as a percentage
	 */
	public int getResistance() {
		int resistance = stat[WISDOM_IDX] +
						 stat[CONSTITUTION_IDX];
		resistance *= 10;
		
		for (int i = 0; i < numItems; i++) {
			resistance -= item[i].getProfanity() * 5;
		}
		
		return resistance;
	}
	
	/**
	 * Determine whether a character manages to evade an active
	 * skill. This method calculates the evade probability and
	 * then rolls a dice to check whether it worked.
	 * 
	 * @return Whether the character manages to evade
	 */
	public boolean getEvade() {
		int prob = getEvadeProb();
		int roll = Dice.roll(100);
		return (roll <= prob);
	}
	
	/**
	 * Returns all active skills that the player has due to
	 * his items. The array has exactly the length as the number
	 * of skills the user has.
	 * 
	 * @return An array of active skills
	 */
	public ActiveSkill[] getActiveSkills() {
		int numSkills = 0;
		
		for (int i = 0; i < numItems; i++) {
			numSkills += item[i].getNumActiveSkills();
		}
		
		ActiveSkill[] skills = new ActiveSkill[numSkills];
		
		int index = 0;
		for (int i = 0; i < numItems; i++) {
			for (int j = 0; j < item[i].getNumActiveSkills(); j++) {
				skills[index] = item[i].getActiveSkill(j);
				index++;
			}
		}
		
		return skills;
	}
}
