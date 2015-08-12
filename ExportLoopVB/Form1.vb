Imports System.IO
Imports System.Text

Public Class Form1
    Private currentProcess As Process

    ''' <summary>
    ''' Quote pathnames in arguments so other Windows applications will understand them.
    ''' Converted from https://github.com/gfxmonk/csharp-quote-argv/blob/master/QuoteArguments.cs
    ''' </summary>
    ''' <param name="arg">An argument to escape</param>
    ''' <returns>The escaped argument</returns>
    Private Shared Function QuoteArgument(arg As String) As String
        Dim backslashes As Integer = 0

        Dim sb As New StringBuilder

        Dim needquote As Boolean = arg.Length = 0 Or arg.Contains(" ") Or arg.Contains(vbTab)
        If needquote Then
            sb.Append(""""c)
        End If

        For Each c As Char In arg
            If c = "\"c Then
                backslashes += 1
            ElseIf c = """"c Then
                ' Make sure pending backslashes are doubled
                sb.Append(New String("\"c, backslashes * 2))
                backslashes = 0

                ' Append a backslash-escaped double quote
                sb.Append("\""")
            Else
                If backslashes > 0 Then
                    sb.Append(New String("\"c, backslashes))
                    backslashes = 0
                End If

                sb.Append(c)
            End If
        Next

        If backslashes > 0 Then
            sb.Append(New String("\"c, backslashes))
        End If

        If needquote Then
            sb.Append(New String("\"c, backslashes))
            sb.Append(""""c)
        End If

        Return sb.ToString()
    End Function

    Private Sub Form1_Load(sender As Object, e As EventArgs) Handles MyBase.Load
        txtOutputDir.Text = Environment.CurrentDirectory
    End Sub

    Private Sub btnAdd_Click(sender As Object, e As EventArgs) Handles btnAdd.Click
        If OpenFileDialog1.ShowDialog = DialogResult.OK Then
            For Each f In OpenFileDialog1.FileNames
                ListView1.Items.Add(f)
            Next
        End If
    End Sub

    Private Sub btnRemove_Click(sender As Object, e As EventArgs) Handles btnRemove.Click
        For Each item As ListViewItem In ListView1.SelectedItems
            ListView1.Items.Remove(item)
        Next
    End Sub

    Private Sub ListView1_DragEnter(sender As Object, e As DragEventArgs) Handles ListView1.DragEnter
        If e.Data.GetDataPresent(DataFormats.FileDrop) Then
            e.Effect = DragDropEffects.Link
        End If
    End Sub

    Private Sub ListView1_DragDrop(sender As Object, e As DragEventArgs) Handles ListView1.DragDrop
        If e.Data.GetDataPresent(DataFormats.FileDrop) Then
            Dim filenames As String() = e.Data.GetData(DataFormats.FileDrop)
            For Each filename As String In filenames
                ListView1.Items.Add(filename)
            Next
        End If
    End Sub

    Private Sub btnBrowse_Click(sender As Object, e As EventArgs) Handles btnBrowse.Click
        If Not String.IsNullOrEmpty(txtOutputDir.Text) Then
            FolderBrowserDialog1.SelectedPath = txtOutputDir.Text
            If FolderBrowserDialog1.ShowDialog = DialogResult.OK Then
                txtOutputDir.Text = FolderBrowserDialog1.SelectedPath
            End If
        End If
    End Sub

    Private Sub btnStart_Click(sender As Object, e As EventArgs) Handles btnStart.Click
        If Not Directory.Exists(txtOutputDir.Text) Then
            MessageBox.Show("The specified output directory does not exist.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error)
            Exit Sub
        End If

        Dim render_bars As Integer = 5
        Dim save_bars As Integer = 1

        Dim multiplier As Integer = render_bars

        Dim args As String = "-vbgui "
        If chk0ToStart.Checked Then
            args += " -0L"
            multiplier += save_bars
        End If
        If chkStartToEnd.Checked Then
            args += " -LE"
            multiplier += save_bars
        End If
        If chk0ToEnd.Checked Then
            args += " -0E"
            multiplier += save_bars
        End If

        ProgressBar1.Value = 0
        ProgressBar1.Maximum = ListView1.Items.Count * multiplier

        pnlReady.Visible = False
        pnlInProgress.Visible = True

        Dim paths As New List(Of String)
        For Each item As ListViewItem In ListView1.Items
            paths.Add(item.Text)
        Next

        Dim startInfo As New ProcessStartInfo("exportloop.exe", args)
        startInfo.RedirectStandardInput = True
        startInfo.RedirectStandardOutput = True
        startInfo.RedirectStandardError = True
        startInfo.ErrorDialog = True
        startInfo.UseShellExecute = False
        startInfo.CreateNoWindow = True
        startInfo.WorkingDirectory = txtOutputDir.Text

        currentProcess = Process.Start(startInfo)
        Dim inp As StreamWriter = currentProcess.StandardInput
        Dim out As StreamReader = currentProcess.StandardOutput
        Dim err As StreamReader = currentProcess.StandardError
        currentProcess.EnableRaisingEvents = True

        AddHandler currentProcess.Exited, AddressOf ProcessFinished

        Dim t As New Task(
            Sub()
                While Not out.EndOfStream
                    Dim text = out.ReadLine
                    Console.WriteLine(text)
                    Me.BeginInvoke(New Action(
                                   Sub()
                                       lblCurrentFile.Text = text
                                       If text.StartsWith("vbgui: ") Then
                                           If text.StartsWith("vbgui: rendering") Then
                                               ProgressBar1.Value += render_bars
                                           ElseIf text.StartsWith("vbgui: saving") Then
                                               ProgressBar1.Value += save_bars
                                           ElseIf text.StartsWith("vbgui: skipped") Then
                                               ProgressBar1.Value += multiplier
                                           ElseIf text.StartsWith("vbgui: finished ") Then
                                               Dim path = text.Substring("vbgui: finished ".Length)
                                               For Each item As ListViewItem In ListView1.Items
                                                   If item.Text = path Then
                                                       ListView1.Items.Remove(item)
                                                       Exit For
                                                   End If
                                               Next
                                           End If
                                       Else
                                           lblCurrentFile.Text = text
                                       End If
                                   End Sub))
                End While
            End Sub)
        t.Start()
        Dim t2 As New Task(
            Sub()
                While Not err.EndOfStream
                    Dim text = err.ReadLine
                    Console.Error.WriteLine(text)
                    MessageBox.Show(text)
                End While
            End Sub)
        t2.Start()

        For Each path As String In paths
            inp.WriteLine(path)
        Next
        inp.WriteLine()
    End Sub

    Private Sub ProcessFinished(sender As Object, e As EventArgs)
        Dim p As Process = CType(sender, Process)
        Me.Invoke(Sub()
                      If currentProcess.ExitCode = 0 Then
                          ListView1.Items.Clear()
                      End If
                      pnlInProgress.Visible = False
                      pnlReady.Visible = True
                      currentProcess = Nothing
                  End Sub)
    End Sub

    Private Sub btnOpenFolder_Click(sender As Object, e As EventArgs) Handles btnOpenFolder.Click
        If Not Directory.Exists(txtOutputDir.Text) Then
            MessageBox.Show("The specified output directory does not exist.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error)
            Exit Sub
        End If

        Process.Start(txtOutputDir.Text)
    End Sub

    Private Sub btnAbout_Click(sender As Object, e As EventArgs) Handles btnAbout.Click
        Dim N = Environment.NewLine
        MessageBox.Show("Export Loop VB - version 1.1.1" & N & "© 2015 libertyernie" & N & N & "https://github.com/libertyernie/vgmstream" & N & N &
                        "Permission to use, copy, modify, and distribute this software for any " &
                        "purpose with or without fee is hereby granted, provided that the above " &
                        "copyright notice and this permission notice appear in all copies." & vbCrLf &
                        vbCrLf &
                        "THE SOFTWARE IS PROVIDED ""AS IS"" AND THE AUTHOR DISCLAIMS ALL WARRANTIES " &
                        "WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF " &
                        "MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR " &
                        "ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES " &
                        "WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN " &
                        "ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF " &
                        "OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.")
    End Sub

    Private Sub btnCancel_Click(sender As Object, e As EventArgs) Handles btnCancel.Click
        If currentProcess IsNot Nothing Then
            currentProcess.Kill()
        End If
    End Sub

    Private Sub Form1_FormClosing(sender As Object, e As FormClosingEventArgs) Handles MyBase.FormClosing
        If currentProcess IsNot Nothing Then
            currentProcess.Kill()
        End If
    End Sub
End Class
