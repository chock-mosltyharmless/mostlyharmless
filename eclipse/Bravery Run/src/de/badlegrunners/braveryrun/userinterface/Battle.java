/**
 * The userinterface package builds the link between gamelogic,
 * graphics and also AI.
 * 
 * The userinterface holds information which interactions the
 * user can make, whether some parts shall be grayed out and also
 * the time within the current animation.
 */
package de.badlegrunners.braveryrun.userinterface;

import de.badlegrunners.braveryrun.gamelogic.ActiveSkill;
import de.badlegrunners.braveryrun.gamelogic.Character;
import de.badlegrunners.braveryrun.gamelogic.Party;

/**
 * The battle class holds all the relevant user experience information
 * for a single fight between two parties.
 * 
 * The battle is created when it starts and shall be discarded once
 * the battle is over and the effects of the battle result were
 * incorporated into the game.
 * 
 * TODO: I need to think about how I cope with timing. Shall I make
 * everything framerate dependent? That might make things awfully
 * slow on machines with slow graphics cards and it may make things
 * super fast if someone turns off vsynch. I need to come up with
 * some plan here... I could make it discrete, but then I need
 * to make sure it is not choppy. I could do this by making upper
 * and lower limits, so that the tick rate does not wobble. And
 * then I need a maximum tick frequency which should at least be
 * 100 Hz, but not too much?
 * The I also need a Tick class?
 * 
 * @author chock
 */
public class Battle {
	/**
	 * The first party that is usually controlled by the player.
	 */
	private final Party firstParty;
	
	/**
	 * The second party that is usually controlled by the AI.
	 */
	private final Party secondParty;
	
	/**
	 * The global state the battle is currently in. The state
	 * determines how things shall be renders, what the passage
	 * of time effects and what interactions the user or the AI
	 * can do.
	 * 
	 * @author chock
	 */
	public enum BattleState {
		/**
		 * No character is active. Time has to pass until the
		 * first character becomes active. This state is
		 * followed by the SELECT_SKILL state.
		 */
		IDLE,
		
		/**
		 * A character has to select a skill. For a player, a choice
		 * shall be visible, the AI will get time to calculate.
		 * This state is followed by SELECT_TARGET or alternatively
		 * IDLE if skill usage was cancelled.
		 */
		SELECT_SKILL,
		
		/**
		 * A character has to select a target with a given skill. For
		 * an AI this state may be skipped, as the target is chosen
		 * while the skill is selected.
		 * This skill is followed by MOVE_TO_TARGET.
		 */
		SELECT_TARGET,
		
		/**
		 * The active character moves to his target to perform the
		 * activated skill. This skill is followed by APPLY_SKILL.
		 */
		MOVE_TO_TARGET,
		
		/**
		 * The skill is being applied. This skill is followed by
		 * MOVE_BACK.
		 */
		APPLY_SKILL,
		
		/**
		 * The character moves back to his starting point after
		 * the skill was executed. This skill is followed by
		 * IDLE.
		 */
		MOVE_BACK,
	};
	
	/**
	 * The current state that the battle is in.
	 */
	private BattleState state;
	
	/**
	 * Time multiplier for the move to target state.
	 */
	private final int moveToTargetTimeMult = 100;
	
	/**
	 * Time multiplier for the apply skill state.
	 */
	private final int applySkillTimeMult = 100;
	
	/**
	 * Time multiplier for the move back skill.
	 */
	private final int moveBackTimeMult = 100;
	
	/**
	 * Maximum time of that the state time may reach before the
	 * state switches.
	 */
	private final int maxStateTime = 10000;
		
	/**
	 * The time within the current state in a range from 0..9999
	 * The state time has different meanings depending on the state.
	 * The state time may have no meaning at all, for example in
	 * IDLE (where the player time is relevant) or while selecting
	 * a skill.
	 */
	private int stateTime;
	
	/**
	 * The index of the currently active character, between -1 and 5.
	 * -1 is only during the IDLE state. 0 through 2 are for the
	 * characters in the first party, 3 through 5 for characters in
	 * the second party.
	 */
	private int activeCharacterIdx;
	
	/**
	 * The index of the currently selected target, between -1 and 5.
	 * -1 is during either of IDLE, SELECT_SKILL or SELECT_TARGET state.
	 * 0 through 2 are for the
	 * characters in the first party, 3 through 5 for characters in
	 * the second party.
	 */
	private int targetCharacterIdx; 
	
	/**
	 * The skill that is activated by the active character. This may
	 * be null if no skill was yet chosen.
	 */
	private ActiveSkill activeSkill;
	
	/**
	 * @return The currently active battle state
	 */
	public BattleState getBattleState() {
		return state;
	}
	
	/**
	 * @return Index of the currently active character.
	 */
	public int getActiveCharacterIdx() {
		return activeCharacterIdx;
	}

	/**
	 * Return character with the given index. This index is
	 * in the range -1..5. For -1, null is returned.
	 * 
	 * @param index Index of the character to retrieve
	 * @return The respective member of one of the parties or null
	 */
	private Character getCharacterByIdx(int index) {
		if (index < 0) {
			return null;
		} else if (index < firstParty.getNumMembers()) {
			return firstParty.getMember(index);
		} else {
			index -= firstParty.getNumMembers();
			return secondParty.getMember(index);
		}
	}
	
	/**
	 * Get the total number of characters that take part in the
	 * battle. This number will always be 6 no matter what.
	 * 
	 * @return Number of characters on the battlefield
	 */
	private int getNumCharacters() {
		return firstParty.getNumMembers() + secondParty.getNumMembers();
	}
	
	/**
	 * Returns the currently active character. This method may return
	 * null if no character is active.
	 * 
	 * @return The active character
	 */
	public Character getActiveCharacter() {
		return getCharacterByIdx(activeCharacterIdx);
	}
	
	/**
	 * Returns the currently targeted character. This method may
	 * return null if no character is currently targeted.
	 * 
	 * @return The currently targeted character
	 */
	public Character getTargetCharacter() {
		return getCharacterByIdx(targetCharacterIdx);
	}
	
	/**
	 * Initiate a new battle. The battle starts in the beginning state
	 * where no player can do anything until the first Character
	 * becomes active.
	 * 
	 * @param first The human player party
	 * @param second The AI party
	 */
	public Battle(Party first, Party second) {
		firstParty = first;
		secondParty = second;
	}
	
	/**
	 * Let time go by in the battle. The effect of passing of time
	 * varies depending on the state that the battle is in. If
	 * the program waits for user input, the display might wobble,
	 * if nothing is done, the active timer of the characters might
	 * move on and if an animation is going, the animation time might
	 * run. 
	 * 
	 * @param ticks
	 */
	public void passTime(int ticks) {
		switch(state) {
		case IDLE:
			// Advance time for all the idling characters
			for (int i = 0; i < getNumCharacters(); i++) {
				getCharacterByIdx(i).passTime(ticks);
			}
			
			// Check if any character is active
			for (int i = 0; i < getNumCharacters(); i++) {
				if (getCharacterByIdx(i).isActivated()) {
					stateTime = 0;
					state = BattleState.SELECT_SKILL;
					activeCharacterIdx = i;
					break;
				}
			}
			break;
			
		case SELECT_SKILL:
			// Do nothing?
			break;
			
		case SELECT_TARGET:
			// Do nothing?
			break;
			
		case MOVE_TO_TARGET:
			// pass time and eventually switch
			stateTime += ticks * moveToTargetTimeMult;
			if (stateTime >= maxStateTime) {
				stateTime = 0;
				state = BattleState.APPLY_SKILL;
			}
			break;
			
		case APPLY_SKILL:
			// pass time and eventually apply effects and switch
			stateTime += ticks * applySkillTimeMult;
			if (stateTime >= maxStateTime) {
				stateTime = 0;
				activeSkill.apply(getActiveCharacter(),
								  getTargetCharacter());
				activeSkill = null;
				state = BattleState.MOVE_BACK;
			}
			break;
			
		case MOVE_BACK:
			// pass time and eventually go back to idle.
			stateTime += ticks * moveBackTimeMult;
			if (stateTime >= maxStateTime) {
				stateTime = 0;
				getActiveCharacter().resetActionTime();
				activeCharacterIdx = -1;
				targetCharacterIdx = -1;
				// TODO: Check whether the battle is over.
				state = BattleState.IDLE;
			}
		}
	}
	
	/**
	 * Selects the currently active skill. This is only allowed during
	 * the SELECT_SKILL state. The index must be in the allowed range
	 * of active skills of the active character.
	 * 
	 * @param index Index of the skill of the active character
	 */
	public void selectActiveSkill(int index) {
		if (state != BattleState.SELECT_SKILL) {
			// Choose invalid index so that an exception occurs
			index = -1;
		}
		activeSkill = getActiveCharacter().getActiveSkills()[index];
		stateTime = 0;
		state = BattleState.SELECT_TARGET;
	}
	
	/**
	 * Select target character by means of his index. This is only
	 * allowed during the SELECT_TARGET state.
	 * 
	 * @param index Index of the character that is chosen as a target.
	 */
	public void selectTarget(int index) {
		if (state != BattleState.SELECT_TARGET) {
			// Choose invalid index so that an exception occurs
			index = -1;
		}
		targetCharacterIdx = index;
		stateTime = 0;
		state = BattleState.MOVE_TO_TARGET;
	}
}
