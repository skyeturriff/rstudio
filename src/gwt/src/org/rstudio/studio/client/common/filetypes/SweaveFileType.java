/*
 * SweaveFileType.java
 *
 * Copyright (C) 2022 by Posit Software, PBC
 *
 * Unless you have received this program directly from Posit Software pursuant
 * to the terms of a commercial license agreement with Posit Software, then
 * this program is licensed to you under the terms of version 3 of the
 * GNU Affero General Public License. This program is distributed WITHOUT
 * ANY EXPRESS OR IMPLIED WARRANTY, INCLUDING THOSE OF NON-INFRINGEMENT,
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. Please refer to the
 * AGPL (http://www.gnu.org/licenses/agpl-3.0.txt) for more details.
 *
 */
package org.rstudio.studio.client.common.filetypes;

import java.util.HashSet;

import org.rstudio.core.client.command.AppCommand;
import org.rstudio.core.client.regex.Pattern;
import org.rstudio.studio.client.RStudioGinjector;
import org.rstudio.studio.client.common.reditor.EditorLanguage;
import org.rstudio.studio.client.workbench.commands.Commands;

import com.google.gwt.resources.client.ImageResource;

public class SweaveFileType extends TextFileType
{
   SweaveFileType(String id,
                  String label,
                  EditorLanguage editorLanguage,
                  String defaultExtension,
                  ImageResource icon)
   {
      super(id, 
            label, 
            editorLanguage, 
            defaultExtension,
            icon,
            WordWrap.DEFAULT,
            false, 
            true, 
            true, 
            false, 
            false,
            false,
            true,
            true,
            false,
            true,
            true,
            false);
   }
   
   @Override
   public boolean getWordWrap()
   {
      return RStudioGinjector.INSTANCE.getUserPrefs().softWrapRmdFiles().getValue();
   }

   @Override
   public HashSet<AppCommand> getSupportedCommands(Commands commands)
   {
      HashSet<AppCommand> result = super.getSupportedCommands(commands);
      result.add(commands.jumpTo());
      result.add(commands.jumpToMatching());
      result.add(commands.goToHelp());
      result.add(commands.goToDefinition());
      result.add(commands.insertRoxygenSkeleton());
      return result;
   }

   @Override
   public Pattern getRnwStartPatternBegin()
   {
      return RNW_START_PATTERN;
   }

   @Override
   public Pattern getRnwStartPatternEnd()
   {
      return RNW_END_PATTERN;
   }

   private static final Pattern RNW_START_PATTERN =
                                                Pattern.create("^\\s*\\<\\<");
   private static final Pattern RNW_END_PATTERN =
                                                Pattern.create("\\>\\>=");
}
