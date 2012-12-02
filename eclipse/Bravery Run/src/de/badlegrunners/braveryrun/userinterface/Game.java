package de.badlegrunners.braveryrun.userinterface;

import java.util.Scanner;
import android.content.Context;

import de.badlegrunners.braveryrun.graphics.BattleRenderer;
import de.badlegrunners.braveryrun.graphics.RenderMachine;
import de.badlegrunners.braveryrun.R;
import de.badlegrunners.braveryrun.gamelogic.Party;
import de.badlegrunners.braveryrun.util.DataScanner;

/**
 * Holds the information of the current game. This includes the game
 * state (party creation, battle, ...) as well as all the data, like
 * the current player party, the level and so on.
 * 
 * @author chock
 */
public class Game {
	/**
	 * All the global states that the game may be in, like
	 * party generation or a battle.
	 * 
	 * @author chock
	 */
	public enum GameState {
		/**
		 * The game is inside a battle.
		 */
		BATTLE
	};
	
	/**
	 * The global state that the game is in at the moment.
	 */
	protected GameState state;
	
	/**
	 * The party of the player. This party never changes throughout
	 * a game session.
	 */
	protected Party playerParty;
	
	/**
	 * The battle object if a battle is running
	 */
	protected Battle battle;
	
	/**
	 * The machine used for rendering. Here we have to specify what
	 * engine to use for rendering.
	 */
	private RenderMachine renderMachine;
	
	/**
	 * Constructs a new game, this would usually result in a party
	 * generation state.
	 * 
	 * @param context The context of the game used for resource loading.
	 */
	public Game(Context context, RenderMachine renderMachine) {
		this.renderMachine = renderMachine;
		
		// Testing: load dataset
        String dataSet = context.getResources().getString(R.string.example_dataset);
		Scanner scan = new Scanner(dataSet);
		DataScanner datScan = new DataScanner(scan);
		try
        {
        	datScan.checkToken("Party", "Main");
        	datScan.checkToken("{", "Main");
        	playerParty = new Party(scan, 1);
        	datScan.checkToken("}", "Main");
        }
        catch (Exception e)
        {
        	System.out.println(e.getMessage());
        }
		
		// Just some example party
		Party enemies = null;
        dataSet = context.getResources().getString(R.string.example_dataset);
		scan = new Scanner(dataSet);
		datScan = new DataScanner(scan);
		try
        {
        	datScan.checkToken("Party", "Main");
        	datScan.checkToken("{", "Main");
        	enemies = new Party(scan, 1);
        	datScan.checkToken("}", "Main");
        }
        catch (Exception e)
        {
        	System.out.println(e.getMessage());
        }
		
		// Set the initial state and its rendering info.
		state = GameState.BATTLE;
		battle = new Battle(playerParty, enemies);
		BattleRenderer renderer = new BattleRenderer(battle,
												     this.renderMachine);
		this.renderMachine.setRenderEngine(renderer);
	}
}
