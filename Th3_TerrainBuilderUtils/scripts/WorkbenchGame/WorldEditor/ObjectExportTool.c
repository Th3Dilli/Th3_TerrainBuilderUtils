[WorkbenchToolAttribute("Export objects", "Export objects", "2", awesomeFontCode: 0xf0d0)]
class ObjectExportTool: WorldEditorTool
{
	[Attribute("", UIWidgets.ResourceNamePicker, "csv where space is the separator", "csv")]
	ResourceName DataPath;
	
	[Attribute("true", UIWidgets.CheckBox, "Makes Y coordinate relative to terrain vs absolute in world")]
	bool PosYRelativeToTerrain;
	[ButtonAttribute("Export objects")]
	void ExportData()
	{
		string exportPath = DataPath.GetPath();
		FileHandle file = FileIO.OpenFile(exportPath , FileMode.WRITE);
		
		if (!file)
		{
			Print("Unable to open file for writting " + exportPath , LogLevel.ERROR);
			return;
		}
		
		Print("Exporting entities");
		
		string format  = "\"%1\" %2 %3 %4 %5 %6 %7 %8 %9";
		
		EditorEntityIterator iter(m_API);
		int approximateCount = m_API.GetEditorEntityCount();
		
		WorldEditor we = Workbench.GetModule(WorldEditor);
		WBProgressDialog progress = new WBProgressDialog("Exporting...", we);
		
		IEntitySource src = iter.GetNext();
		while (src)
		{
			IEntity entity = m_API.SourceToEntity(src);
			string resourceName = entity.GetPrefabData().GetPrefab().GetResourceName();
			if(entity && resourceName) {
				vector mat[4];
				entity.GetTransform(mat);
				vector pos = mat[3];
				vector rot = mat[2];
				float scale = entity.GetScale();
				
				if (PosYRelativeToTerrain)
				{
					pos[1] = pos[1] - m_API.GetTerrainSurfaceY(pos[0], pos[2]);
				}
				
				pos[2] = -pos[2];
				float quat[4];
				vector matt[3];
				vector angles = entity.GetAngles();
				float x = angles[0];
				angles[0] = -angles[1];
				angles[1] = -x;
				
				Math3D.AnglesToMatrix(angles, matt);
				Math3D.MatrixToQuat(matt, quat);
				
				// resourceHash
				// posX
				// posY
				// posZ
				// quatX
				// quatY
				// quatZ
				// quatRotation
				// scale
				file.FPrintln(string.Format(format, resourceName, pos[0] , pos[1], pos[2], quat[0], quat[1], quat[2], quat[3], scale));
				//file.FPrintln(string.Format(format, resourceName, pos[0].ToString(-1,6) , pos[1].ToString(-1,6), pos[2].ToString(-1,6), quat[0].ToString(-1,6), quat[1].ToString(-1,6), quat[2].ToString(-1,6), quat[3].ToString(-1,6), scale.ToString(-1,6)));
			}
			
			src = iter.GetNext();
			progress.SetProgress(iter.GetCurrentIdx() / approximateCount);
		}
		
				
		Print("Export done");
		if (file)
		{
			file.CloseFile();
		}
	}
}