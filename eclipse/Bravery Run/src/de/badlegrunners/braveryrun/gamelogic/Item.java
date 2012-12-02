package de.badlegrunners.braveryrun.gamelogic;

import de.badlegrunners.braveryrun.gamelogic.ActiveSkill;
import de.badlegrunners.braveryrun.util.DataScanner;

import java.util.Scanner;

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
	final protected int generatorSeedValue;
	
	/**
	 * Size of the item.
	 */
	final protected int itemSize;
		
	/**
	 * The number of active skills that are present in this item.
	 */
	final protected int numActiveSkills;
	
	/**
	 * The list of skills. No element may be null. If there
	 * is no active skill, the whole array is null.
	 */
	final protected ActiveSkill activeSkill[];
	
	/**
	 * Passive skill encumberance. This skill has a negative effect
	 * on the evasiveness of the character.
	 */
	final protected int encumberance;
	
	/**
	 * Passive skill defense. This skill is used for blocking
	 * physical damage.
	 */
	final protected int defense;

	/**
	 * Passive skill profanity. This skill has a negative effect on
	 * the resistance and the magical abilities of the character.
	 */
	final protected int profanity;
	
	/**
	 * @return encumberance
	 */
	public int getEncumberance() {
		return encumberance;
	}
	
	/**
	 * @return Defense value of the item
	 */
	public int getDefense() {
		return defense;
	}
	
	/**
	 * @return Profanity value of the item
	 */
	public int getProfanity() {
		return profanity;
	}
	
	/**
	 * @return Number of active skills on this item.
	 */
	public int getNumActiveSkills() {
		return numActiveSkills;
	}
	
	/**
	 * Get an active skill. The index of the skill must be in
	 * the range 0..(getNumActiveSkills()-1).
	 * 
	 * @param index Index of the active skill to retrieve
	 * @return The active skill with the given index
	 */
	public ActiveSkill getActiveSkill(int index) {
		return activeSkill[index];
	}
	
	/**
	 * Randomly generate an item of the given type.
	 * TODO: I need some hints how to do this. Like quality of the
	 * tool and for what type of person this is suited. Maybe
	 * I pass the character properties, and then go from there?
	 * 
	 * @param type Type of the object to generate.
	 * @param itemSize Size of the item.
	 */
	public Item(ItemType type, int itemSize) {
		this.itemType = type;
		this.generatorSeedValue = 0; // TODO: Implement non-cheater
		
		// Item skills
		this.numActiveSkills = 0;
		this.itemSize = itemSize;
		this.activeSkill = null;
		this.encumberance = 0;
		this.defense = 0;
		this.profanity = 0;
	}
	
	/**
	 * Loads an Item from a text data file using a scanner object.
	 * The scanner must point to the start of the item after the
	 * opening brace. The scanner is put right before the closing brace
	 * after loading is completed.
	 * 
	 * @param scanner The scanner object that holds the data set
	 * @param version The version of the data set
	 * @throws Exception If dataset is corrupt
	 */
	public Item(Scanner scanner, int version) throws Exception {
		if (version != 1) {
			throw new Exception("Item: Unknown data set version:" +
							    version);
		}
		
		DataScanner datScan = new DataScanner(scanner);
		datScan.checkToken("itemType", "Item");
		String type = scanner.next();
		if (type.equals("PRIMARY_TOOL")) {itemType = ItemType.PRIMARY_TOOL;}
		else if (type.equals("TWOHANDED_TOOL")) {itemType = ItemType.TWOHANDED_TOOL;}
		else if (type.equals("SECONDARY_TOOL")) {itemType = ItemType.SECONDARY_TOOL;}
		else if (type.equals("ARMOR")) {itemType = ItemType.ARMOR;}
		else if (type.equals("HELMET")) {itemType = ItemType.HELMET;}
		else {
			throw new Exception("Item: Unknown type: " + type);
		}

		generatorSeedValue = datScan.getNextInt("generatorSeedValue",
											    "Item");
		itemSize = datScan.getNextInt("itemSize", "Item");
		numActiveSkills = datScan.getNextInt("numActiveSkills",
											 "Item");
		
		// Load the active skills
		activeSkill = new ActiveSkill[numActiveSkills];
		for (int i = 0; i < numActiveSkills; i++) {
			datScan.checkToken("ActiveSkill", "Item");
			datScan.checkToken("{", "Item");
			activeSkill[i] = new ActiveSkill(scanner, version);
			datScan.checkToken("}", "Item");
		}
		
		// Load the passive skills
		encumberance = datScan.getNextInt("encumberance", "Item");
		defense = datScan.getNextInt("defense", "Item");
		profanity = datScan.getNextInt("profanity", "Item");
	}
}
