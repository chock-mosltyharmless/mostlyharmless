package de.badlegrunners.braveryrun.gamelogic;

import de.badlegrunners.braveryrun.gamelogic.SkillModifiers;
import de.badlegrunners.braveryrun.util.DataScanner;

import java.util.Scanner;

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
	protected final int generatorSeedValue;
	
	/**
	 * Skill type identifier. This determines the type of the skill
	 * and its logo. This also may have some influence on the
	 * animation and the effect with the skill? But it is also
	 * set by the tool.
	 */
	protected final int skillTypeID;
	
	/**
	 * The type of effect that the skill causes. Each skill type ID
	 * has exactly one type, but many IDs may share a type.
	 * Example: Slash and Pierce have different skill type IDs
	 * but the same effect type (physical damage)
	 * 
	 * @author chock
	 */
	public enum EffectType {
		/**
		 * Skill heals one ally. Skill is not evaded? But skill might
		 * be resisted?
		 */
		HEAL,

		/**
		 * Skill causes physical damage. The skill may be evaded and
		 * is blocked by armor.
		 */
		PHYSICAL_DAMAGE,
		
		/**
		 * Skill causes magical damage. The skill may be evaded and
		 * resisted.
		 */
		MAGICAL_DAMAGE
	};
	
	/**
	 * The effect that this skill causes.
	 */
	private final EffectType effect;
	
	/**
	 * The quality of the item. This needs to be saved for non-cheating
	 * abilities.
	 */
	protected final int quality;
	
	/**
	 * The size of the item this skill is used on.
	 */
	protected final int itemSize;
	
	/**
	 * The modifiers from character stats that affect how much
	 * the skill does.
	 */
	protected final SkillModifiers skillModifiers;
	
	/**
	 * Get the effect of this skill
	 */
	public EffectType getEffect() {return effect;}
	
	/**
	 * Core constructor of an active skill.
	 * @param seed Random number generator for the seed value
	 * @param skillID The type of base skill to use for generation
	 * @param quality Quality of the skill. The higher the number,
	 * 		  the better the skill will turn out.
	 * @param itemSize The size of the tool that this skill is on.
	 * 		  The larger the size the more powerful it should be.
	 * @param effect The effect that this skill causes when used.
	 */
	public ActiveSkill(int seed, int skillID, int quality, int itemSize,
					   EffectType effect) {
		this.generatorSeedValue = seed;
		this.skillTypeID = skillID;
		this.quality = quality;
		this.itemSize = itemSize;
		this.effect = effect;
		skillModifiers = new SkillModifiers();
	}
	
	/**
	 * Load an active skill from a text file using a java.util.Scanner
	 * @param Scanner A scanner object initialized on some text data
	 * 		  that has to start with this skill. The text format has
	 *        yet to be specified properly.
	 * @param version Version of the dataset. Used for downward
	 * 		  compatibility.
	 * @throws Exception when dataset is corrupted.
	 */
	public ActiveSkill(Scanner scanner, int version) throws Exception {
		/* TODO: Make own exception */
		if (version != 1) {
			throw new Exception("ActiveSkill: Active skill version " + version + "not supported.");
		}

		DataScanner datScan = new DataScanner(scanner);
		
		generatorSeedValue = datScan.getNextInt("generatorSeedValue",
												"ActiveSkill");
		skillTypeID = datScan.getNextInt("skillTypeID", "ActiveSkill");
		quality = datScan.getNextInt("quality", "ActiveSkill");
		itemSize = datScan.getNextInt("itemSize", "ActiveSkill");
		datScan.checkToken("effect", "ActiveSkill");
		String effect = scanner.next();
		if (effect.equals("HEAL")) {this.effect = EffectType.HEAL;}
		else if (effect.equals("PHYSICAL_DAMAGE")) {this.effect = EffectType.PHYSICAL_DAMAGE;}
		else if (effect.equals("MAGICAL_DAMAGE")) {this.effect = EffectType.MAGICAL_DAMAGE;}
		else {
			throw new Exception("ActiveSkill: Effect type " + effect + "unknown.");
		}
		
		// Get the embracing brackets for the modifiers
		datScan.checkToken("SkillModifiers", "ActiveSkill");
		datScan.checkToken("{", "ActiveSkill");
		skillModifiers = new SkillModifiers(scanner, version);
		datScan.checkToken("}", "ActiveSkill");
	}
}
