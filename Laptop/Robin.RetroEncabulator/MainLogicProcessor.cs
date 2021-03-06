﻿using System;
using System.Collections.Generic;
using System.ComponentModel.Composition;
using System.Diagnostics;
using System.Drawing;
using System.Media;
using System.Text;
using Robin.Core;
using Stateless;
using System.Timers;

namespace Robin.RetroEncabulator
{
	[ExportMetadata("Name", "RetroEncabulator")]
	[Export(typeof(IRobotController))]
	public class MainLogicProcessor : IRobotController
	{
		private readonly StateMachine<State, Trigger> stateMachine;
		private static readonly Timer timer = new Timer();
		private static readonly Stopwatch stopwatch = new Stopwatch();
		private static SoundPlayer soundPlayer = new SoundPlayer();
		private static readonly Colors[] AllColors = (Colors[])Enum.GetValues(typeof(Colors));
		private static int colorsIndex = 0;
		private static long stopwatchTime;
		
		public MainLogicProcessor()
		{
			VisionData = new VisionData();
			SensorData = new SensorData();
			LogicState = new LogicState();
			
			stateMachine = new StateMachine<State, Trigger>(State.Idle);

			stateMachine.OnUnhandledTrigger((state, trigger) => { });

			stateMachine.Configure(State.Idle)
				.Permit(Trigger.PoweredUp, State.Starting)
				.OnEntry(() => Commander.SetColors(Colors.Yellow));

			stateMachine.Configure(State.Starting)
				.Permit(Trigger.Finished, State.LookingForBall)
				.Permit(Trigger.BallCaught, State.FindingGoal)
				.OnEntry(() =>
				{
					stopwatchTime = stopwatch.ElapsedMilliseconds;
					SoundClipPlayer.PlayIntro();
					Commander.SetColors(Colors.Cyan);
				});

			stateMachine.Configure(State.LookingForBall)
				.Permit(Trigger.CameraLockedOnBall, State.ClosingInOnBall)
				.Permit(Trigger.BallCaught, State.FindingGoal)
				.OnEntry(
				() =>
				{
					Commander.SetColors(Colors.Blue);
					stopwatchTime = stopwatch.ElapsedMilliseconds;
				});

			stateMachine.Configure(State.ClosingInOnBall)
				.Permit(Trigger.CameraLostBall, State.LookingForBall)
				.Permit(Trigger.BallCaught, State.FindingGoal)
				.Permit(Trigger.Timeout, State.LookingForBall)
				.OnEntry(() =>
				{
					StartTimer(10000, Trigger.Timeout);
					Commander.SetColors(Colors.Red);
				})
				.OnExit(StopTimer);

			stateMachine.Configure(State.FindingGoal)
				.Permit(Trigger.CoilgunLaunched, State.LookingForBall)
				.Permit(Trigger.BallLost, State.LookingForBall)
				.Permit(Trigger.Timeout, State.LookingForBall)
				.OnEntry(() =>
				{
					LogicState.FindingGoal = true;
					StartTimer(5000, LaunchBall);
					Commander.SetColors(Colors.Magenta);
				})
				.OnExit(
				() =>
				{
					LogicState.FindingGoal = false;
					StopTimer();
				});

			stopwatch.Start();
		}

		public VisionData VisionData { get; set; }

		public SensorData SensorData { get; set; }
		
		public IRobotCommander Commander { get; set; }

		public LogicState LogicState { get; set; }

		public IntPtr Parent { get; set; }

		private void StartTimer(double milliseconds, Action action)
		{
			timer.AutoReset = false;
			timer.Stop();
			timer.Interval = milliseconds;
			timer.Start();
			timer.Elapsed += (sender, args) => action();
		}

		private void StartTimer(double milliseconds, Trigger trigger)
		{
			StartTimer(milliseconds, () => stateMachine.Fire(trigger));
		}

		private void StopTimer()
		{
			timer.Stop();
		}

		public void Update()
		{
			switch (stateMachine.State)
			{
				case State.Idle:
					Idle();
					break;
				case State.Starting:
					Starting();
					break;
				case State.LookingForBall:
					LookingForBall();
					break;
				case State.ClosingInOnBall:
					ClosingInOnBall();
					break;
				case State.FindingGoal:
					FindingGoal();
					break;
			}

			// HACK: Testime
			System.Threading.Thread.Sleep(100);
		}

		private int ledToggleNext = 0;
		private void Idle()
		{
			if (SensorData.IsPowered) { 
				stateMachine.Fire(Trigger.PoweredUp);
			}

			// Toggle through different colors
			/*if (stopwatch.ElapsedMilliseconds > ledToggleNext)
			{
				ToggleLeds();
				ledToggleNext += 2000;
			}*/
		}

		private void Starting()
		{
			if (stopwatch.ElapsedMilliseconds < stopwatchTime + 1500)
				Commander.Move(0, 200);
			else
				stateMachine.Fire(Trigger.Finished);
		}

		private void LookingForBall()
		{
			if (SensorData.BallInDribbler)
				stateMachine.Fire(Trigger.BallCaught);

			if (VisionData.TrackingBall)
				stateMachine.Fire(Trigger.CameraLockedOnBall);

			if (stopwatch.ElapsedMilliseconds < stopwatchTime + 5000)
			{
				//Commander.Stop();
				Commander.Turn(50);
			}
			else if (stopwatch.ElapsedMilliseconds < stopwatchTime + 6000)
			{
				Commander.Stop();
				Commander.Move(0, 300);
			}
			else
				stopwatchTime = stopwatch.ElapsedMilliseconds + 5000;
		}

		private void ClosingInOnBall()
		{
			if (!VisionData.TrackingBall)
				stateMachine.Fire(Trigger.CameraLostBall);

			if (SensorData.BallInDribbler)
				stateMachine.Fire(Trigger.BallCaught);

			Commander.Stop();
			Commander.MoveToVisionLocation(VisionData.TrackedBallLocation);
		}

		private void FindingGoal()
		{
			var beaconInFront = SensorData.OpponentBeaconFound && Math.Abs(SensorData.BeaconServoDirection) < 10;
			if (beaconInFront || VisionData.OpponentGoalInFront)
			{
				if (!VisionData.FrontBallPathObstructed) {
					LaunchBall();
					return;
				}
			}

			if (!SensorData.BallInDribbler)
				stateMachine.Fire(Trigger.BallLost);

			var direction = SensorData.EstimatedGlobalDirection % 360;
			if (direction < 0)
				direction += 360;

			if ((direction < 90 || direction > 270) && VisionData.OpponentGoalRectangle != Rectangle.Empty)
			{
				SoundClipPlayer.PlayAlarm();
				Commander.SetColors(Colors.Green);

				if (VisionData.OpponentGoalOffset != null)
				{
					Commander.Stop();
					Commander.Turn((short) (VisionData.OpponentGoalOffset.Value * 10));
				}
			}
			else if (direction < 5 || direction > 355)
			{
				var isOnLeftSide = SensorData.EstimatedGlobalPosition.X < -1000;
				Commander.Move((short)(isOnLeftSide ? 270 : 90), 200);
			}
			else
			{
				Commander.Stop();
				Commander.TurnTowardsZero(direction);
			}
			//Commander.MoveAndTurn(0, 0, SensorData.BeaconServoDirection < 0 ? (short)-100 : (short)100);
		}

		private void LaunchBall()
		{
			Commander.FireCoilgun(50);
			Commander.SetColors(Colors.White);
			stateMachine.Fire(Trigger.CoilgunLaunched);
		}

		private readonly Dictionary<State, State> sources = new Dictionary<State, State>();
		public string ToDebugString()
		{
			//return stateMachine.ToString();
			var str = new StringBuilder();

			stateMachine.Configure(State.LookingForBall)
				.OnEntry(StoreSourceState)
				.PermitDynamic(Trigger.DebugBack, () => GetSource(State.LookingForBall))
				.Permit(Trigger.DebugForward, State.ClosingInOnBall)
				.OnEntry(() => { });
			stateMachine.Configure(State.ClosingInOnBall)
				.OnEntry(StoreSourceState)
				.PermitDynamic(Trigger.DebugBack, () => GetSource(State.ClosingInOnBall))
				.Permit(Trigger.DebugForward, State.FindingGoal)
				.OnEntry(() => { });
			stateMachine.Configure(State.FindingGoal)
				.OnEntry(StoreSourceState)
				.PermitDynamic(Trigger.DebugBack, () => GetSource(State.FindingGoal))
				.Permit(Trigger.DebugForward, State.LookingForBall)
				.OnEntry(() => { });

			var allStates = Enum.GetValues(typeof(State));
			var allTriggers = Enum.GetValues(typeof(Trigger));
			for (int i = 0; i < allStates.Length; i++)
			{
				var state = stateMachine.State;

				foreach (Trigger trigger in allTriggers)
				{
					if (trigger == Trigger.DebugBack || trigger == Trigger.DebugForward) continue;
					if (!stateMachine.CanFire(trigger)) continue;

					stateMachine.Fire(trigger);
					str.AppendFormat("[{0}]-{1}->[{2}]", state, trigger, stateMachine.State);
					str.AppendLine();
					stateMachine.Fire(Trigger.DebugBack);
				}

				stateMachine.Fire(Trigger.DebugForward);
			}
			return str.ToString();
		}

		private void StoreSourceState(StateMachine<State, Trigger>.Transition transition)
		{
			if (!sources.ContainsKey(transition.Destination))
				sources.Add(transition.Destination, transition.Source);
			else
				sources[transition.Destination] = transition.Source;
		}

		private State GetSource(State destination)
		{
			var source = sources[destination];
			return source;
		}

		public void Dispose()
		{
			
		}

		private void ToggleLeds()
		{
			Commander.SetColors(AllColors[colorsIndex]);
			colorsIndex++;
			if (colorsIndex == AllColors.Length)
				colorsIndex = 0;
		}
	}
}