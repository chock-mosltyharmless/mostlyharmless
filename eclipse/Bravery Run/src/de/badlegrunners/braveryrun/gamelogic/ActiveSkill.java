package de.badlegrunners.braveryrun.gamelogic;

/**
 * A skill of a tool that can be chosen in actively in combat.
 * The skill can be applied on any character (including oneself).
 * 
 * I want the skills to be completely generic so that I can load
 * basic skills from a skill file (text file?)
 * This file needs to be parsed...
 * 
 * Each skill type may be only used for one of the four different item
 * groups or else it might happen that I need the same logo for different
 * skills.
 * 
 * I have to randomly generate a skill using a seed value and some
 * hints on quality and stuff like that.
 * 
 * TODO: I should do some non-cheating stuff similar to the stuff
 * in Item.
 * 
 * @author chock
 */
public class ActiveSkill {

	/**
	 * TODO: Implement the non-cheating idea.
	 */
	final int generatorSeedValue;
	
	/**
	 * Skill type identifier. This determines the type of the skill
	 * and its logo. 
	 */
	final int skillTypeID;
	
	/**
	 * The quality of the item. This needs to be saved for non-cheating
	 * abilities.
	 */
	final int quality;
	
	/**
	 * The size of the item this skill is used on.
	 */
	final int itemSize;

	/**
	 * Core constructor of an active skill.
	 * @param seed Random number generator for the seed value
	 * @param skillID The type of base skill to use for generation
	 * @param quality Quality of the skill. The higher the number,
	 * 		  the better the skill will turn out.
	 * @param itemSize The size of the tool that this skill is on.
	 * 		  The larger the size the more powerful it should be.
	 */
	public ActiveSkill(int seed, int skillID, int quality, int itemSize) {
		this.generatorSeedValue = seed;
		this.skillTypeID = skillID;
		this.quality = quality;
		this.itemSize = itemSize;
	}
}
