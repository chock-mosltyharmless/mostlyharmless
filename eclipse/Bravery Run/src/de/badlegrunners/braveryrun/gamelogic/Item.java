package de.badlegrunners.braveryrun.gamelogic;

import de.badlegrunners.braveryrun.gamelogic.ActiveSkill;

/**
 * This is the class for any icon, no matter whether it is
 * a primary or a secondary tool, an armor or a helmet.
 * 
 * TODO: The concept is the following: I have a base set of items.
 * These items are defined in a configuration file (text based?)
 * These base sets tell which active and passive skills the item may
 * have and the possibilities for that. Also it has some information
 * on typically required size? I need to be able to parse that, of course.
 * 
 * TODO: I should put in some protection scheme. The idea is the following:
 * I store the seed value of the random number generator together with
 * the item. Then I can recreate the weapon using the random number
 * generator. That ensures, that the item was created with the correct
 * resource file and that the values were not cheated. When loading the
 * item, the stuff is generated again. If it does not fit, I do not
 * allow the item, and the game will not load. I should reload this
 * continually.
 * Now one can still change the tools basics sheet and the items at
 * the same time. So I also need a scheme to check whether the
 * sheet is OK...
 * 
 * @author chock
 */
public class Item {
	/**
	 * Type of the item. This determines into which item slot this
	 * item can be placed. It does not necessarily determine any
	 * properties of the item, or maybe for armor?
	 * 
	 * @author chock
	 */
	public enum ItemType {
		/**
		 * Item carried in the right hand, usually a weapon.
		 * This is not a two-handed weapon, i.e. the secondary weapon
		 * may be used if this tool is used.
		 */
		PRIMARY_TOOL,
		
		/**
		 * Item carried in both hands (but stored in the same slot as
		 * a primary weapon. If such a weapon is wielded, a secondary
		 * weapon may still be carried, but it is not used.
		 */
		TWOHANDED_TOOL,
		
		/**
		 * Item carried in the left hand, usually a shield or a
		 * small weapon.
		 */
		SECONDARY_TOOL,
		
		/**
		 * Item worn on the body, this is some sort of armor.
		 */
		ARMOR,
		
		/**
		 * Something worn on the head. For some reason, everyone
		 * wears something...
		 */
		HELMET
	};
	
	/**
	 * The type of item this is. The type of a tool never changes.
	 */
	final protected ItemType itemType;
	
	/**
	 * TODO: Implement the non-cheating idea.
	 */
	final int generatorSeedValue;
	
	/**
	 * The first skill of the item. This may be null for anything but
	 * for PRIMARY_TOOL and TWOHANDED_TOOL skills. This logic is not
	 * stored in the game engine, but somehow in the scripts.
	 */
	protected ActiveSkill firstSkill;
	
	/**
	 * The optional secondary skill. This skill must be null if the
	 * firstSkill is null. If the first skill is not null, this skill
	 * may or may not be null. This is somehow encoded into the
	 * scripts.
	 */
	protected ActiveSkill secondSkill;
	
	/**
	 * Randomly generate an item of the given type.
	 * TODO: I need some hints how to do this. Like quality of the
	 * tool and for what type of person this is suited. Maybe
	 * I pass the character properties, and then go from there?
	 * 
	 * @param type Type of the object to generate.
	 */
	public Item(ItemType type) {
		this.itemType = type;
		this.generatorSeedValue = 0; // TODO: Implement non-cheater
		
		// Item skills
		firstSkill = null;
		secondSkill = null;
	}
}
