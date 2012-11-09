package de.badlegrunners.braveryrun.gamelogic;

import de.badlegrunners.braveryrun.util.DataScanner;

import java.util.Scanner;

/**
 * Virtual superclass for active and passive skills.
 * 
 * @author chock
 */
public class SkillModifiers {
	/**
	 * The maximum number of modifiers for the effect.
	 * A modifier is a skill of the character that does something
	 * with the skill. The thing that really happens is defined
	 * elsewhere.
	 */
	private final int maxNumModifiers = 3;
	
	/**
	 * The modifier skills of the event. This is an index of the
	 * skill. Thus, skills must be integers 0..Character.numSkills-1
	 * This may also be negative meaning that the modifier is not used.
	 * If index i is negative, i+1 must also be negative.
	 */
	protected int modifierStat[];
	
	/**
	 * The modifier skill multipliers for the modifier. This is an
	 * integer. The multiplier must be zero iff the modifier skill is
	 * negative.
	 */
	protected int modifierMultiplier[];
	
	/**
	 * Constructs the common parts of active and passive skills
	 */
	public SkillModifiers() {
		modifierStat = new int[maxNumModifiers];
		modifierMultiplier = new int[maxNumModifiers];
		
		/* TODO: Set the values! */
	}
	
	/**
	 * Load the common skill values from a text-based dataset using
	 * a Scanner object. The scanner must point to the relevant position.
	 * As of now, this are the modifier values.
	 * 
	 * These are three pairs of skillID, multiplier
	 * 
	 * @param scanner The scanner object that points to the text
	 * @param version The version of the dataset (for compatibility)
	 * @throws Exception If something was fishy
	 */
	public SkillModifiers(Scanner scanner, int version) throws Exception {
		/* TODO: Make own exception */
		if (version != 1) {
			throw new Exception("Skill: Skill version " + version +
								"not supported.");
		}
		
		modifierStat = new int[maxNumModifiers];
		modifierMultiplier = new int[maxNumModifiers];
		
		DataScanner datScan = new DataScanner(scanner);
		datScan.checkToken("modifiers", "Skill");
		
		for (int i = 0; i < maxNumModifiers; i++) {
			modifierStat[i] = scanner.nextInt();
			modifierMultiplier[i] = scanner.nextInt();
		}
	}
	
	/**
	 * For a given skill, this function calculates the amount
	 * that the skill generates for the given character.
	 * The skill values of the character are multiplied by the
	 * modifiers and added up.
	 * @param character The character who uses that skill
	 * @return The value of the skill (damage, healing amount, ...)
	 */
	public int getSkillValue(Character character) {
		int skillValue = 0;
		
		for (int i = 0; i < maxNumModifiers; i++) {
			int statID = modifierStat[i];
			if (statID >= 0) {
				int charStat = character.getStat(statID);
				skillValue += modifierMultiplier[i] * charStat;
			}
		}
		
		return skillValue;
	}	
}
