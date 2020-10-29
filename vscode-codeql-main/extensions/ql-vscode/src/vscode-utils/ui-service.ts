import { TreeDataProvider, window } from 'vscode';
import { DisposableObject } from './disposable-object';
import { commandRunner } from '../helpers';

/**
 * A VS Code service that interacts with the UI, including handling commands.
 */
export class UIService extends DisposableObject {
  protected constructor() {
    super();
  }

  /**
   * Registers a command handler with Visual Studio Code.
   * @param command The ID of the command to register.
   * @param callback Callback function to implement the command.
   * @remarks The command handler is automatically unregistered when the service is disposed.
   */
  protected registerCommand(command: string, callback: (...args: any[]) => any): void {
    this.push(commandRunner(command, callback.bind(this)));
  }

  protected registerTreeDataProvider<T>(viewId: string, treeDataProvider: TreeDataProvider<T>):
    void {

    this.push(window.registerTreeDataProvider<T>(viewId, treeDataProvider));
  }
}
